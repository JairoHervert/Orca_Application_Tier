#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <optional>

#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/entities/User.entity.hpp"
#include "../domain/entities/Repository.entity.hpp"

class ListUserAccessibleProjectsUseCase {
public:
   explicit ListUserAccessibleProjectsUseCase(
      IProjectRepositoryDB &projectRepositoryDB,
      IUserRepository      &userRepository
   )
      : projectRepositoryDB_(projectRepositoryDB),
        userRepository_(userRepository) {}

   std::vector<Repository> execute(
      const std::string &userEmail,
      const std::string &userPassword
   ) {
      std::optional<User> userOpt;
      try {
         // 1. Verificar usuario
         userOpt = userRepository_.findByEmail(userEmail);
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

         // 2. Determinar si es senior
         bool isSenior = userRepository_.isSeniorUser(userEmail);

         std::vector<Repository> projects;
         if (isSenior) {
            // Senior -> ve todos los proyectos
            projects = projectRepositoryDB_.getAllProjects();
         } else {
            // No senior -> solo los que es dueño o está asociado
            projects = projectRepositoryDB_.getProjectsForUser(idUser);
         }

         // 3. Registrar commit exitoso
         projectRepositoryDB_.addCommit(
            idUser,
            std::nullopt,
            std::nullopt,
            true,
            "LIST_ACCESSIBLE_PROJECTS",
            std::string("User ") + userEmail +
               " listed accessible projects (" +
               (isSenior ? "senior: all projects" : "owned or member") +
               ")"
         );

         return projects;

      } catch (const std::exception &e) {
         int idUser = -1;
         if (userOpt.has_value()) {
            idUser = userOpt->idUser;
         }

         // Registrar commit de error
         projectRepositoryDB_.addCommit(
            idUser,
            std::nullopt,
            std::nullopt,
            false,
            "LIST_ACCESSIBLE_PROJECTS_ERROR",
            std::string("Failed to list accessible projects for user ") +
               userEmail + " : " + e.what()
         );

         throw;  // Dejar que la capa superior maneje el error
      }
   }

private:
   IProjectRepositoryDB &projectRepositoryDB_;
   IUserRepository      &userRepository_;
};
