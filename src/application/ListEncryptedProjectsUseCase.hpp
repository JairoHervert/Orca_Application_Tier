#pragma once
#include <string>
#include <stdexcept>
#include <vector>
#include <optional>
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/entities/User.entity.hpp"
#include "../domain/entities/Repository.entity.hpp"

class ListEncryptedProjectsUseCase {
public:
   explicit ListEncryptedProjectsUseCase(
      IProjectRepositoryDB &projectRepositoryDB,
      IUserRepository      &userRepository
   )
      : projectRepositoryDB_(projectRepositoryDB),
        userRepository_(userRepository) {}

   std::vector<Repository> execute(
      const std::string &userEmail,
      const std::string &userPassword
   ) {
      // 1. Verificar usuario
      auto userOpt = userRepository_.findByEmail(userEmail);
      if (!userOpt.has_value()) {
         throw std::runtime_error("User not found: " + userEmail);
      }

      int idUser = userOpt->idUser;

      if (!userRepository_.isValidPassword(userEmail, userPassword)) {
         throw std::runtime_error("Invalid password for user: " + userEmail);
      }

      if (!userRepository_.isVerifiedUser(userEmail)) {
         throw std::runtime_error("User is not verified: " + userEmail);
      }

      if (!userRepository_.isStatusActive(userEmail)) {
         throw std::runtime_error("User is not active: " + userEmail);
      }

      // 2. Verificar rol (solo líder o senior)
      bool isLeader = false;
      bool isSenior = false;

      // Asumiendo que ya tienes estos métodos en IUserRepository
      // (igual que usas isSeniorUser en otros casos de uso)
      if (userRepository_.isSeniorUser(userEmail)) {
         isSenior = true;
      }
      if (userRepository_.isLeaderUser(userEmail)) {
         isLeader = true;
      }

      if (!isLeader && !isSenior) {
         // Registrar commit de intento fallido
         projectRepositoryDB_.addCommit(
            idUser,
            std::nullopt,
            std::nullopt,
            false,
            "LIST_ENCRYPTED_PROJECTS",
            "Unauthorized attempt to list encrypted projects by user " + userEmail
         );

         throw std::runtime_error(
            "User is not allowed to list encrypted repositories (only leader/senior)."
         );
      }

      // 3. Obtener proyectos cifrados
      std::vector<Repository> projects =
         projectRepositoryDB_.getEncryptedProjects();

      // 4. Registrar commit exitoso
      projectRepositoryDB_.addCommit(
         idUser,
         std::nullopt,
         std::nullopt,
         true,
         "LIST_ENCRYPTED_PROJECTS",
         "User " + userEmail + " listed encrypted projects"
      );

      return projects;
   }

private:
   IProjectRepositoryDB &projectRepositoryDB_;
   IUserRepository      &userRepository_;
};
