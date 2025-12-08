#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <optional>

#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/entities/User.entity.hpp"
#include "../domain/entities/Repository.entity.hpp"
#include "../domain/entities/SourceFileDB.entity.hpp"

class ListUserFilesInProjectUseCase {
public:
   explicit ListUserFilesInProjectUseCase(
      IProjectRepositoryDB &projectRepositoryDB,
      IUserRepository      &userRepository
   )
      : projectRepositoryDB_(projectRepositoryDB),
        userRepository_(userRepository) {}

   std::vector<SourceFileDB> execute(
      const std::string &userEmail,
      const std::string &userPassword,
      const std::string &projectName
   ) {
      // ----------------------------
      // 1. Verificar usuario
      // ----------------------------
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

      bool isSenior = userRepository_.isSeniorUser(userEmail);

      // ----------------------------
      // 2. Verificar proyecto
      // ----------------------------
      auto projectOpt = projectRepositoryDB_.findByName(projectName);
      if (!projectOpt.has_value()) {
         throw std::runtime_error("Project not found: " + projectName);
      }

      int idProject = projectOpt->idProject;
      int ownerId   = projectOpt->ownerId;

      // ----------------------------
      // 3. Verificar que el usuario tenga relación con el proyecto
      //    (miembro, owner o senior)
      // ----------------------------
      bool isMember = projectRepositoryDB_.existsUserInProject(idProject, idUser);
      bool isOwner  = (ownerId == idUser);

      if (!isMember && !isOwner && !isSenior) {
         throw std::runtime_error(
            "User does not have access to project: " + projectName
         );
      }

      // ----------------------------
      // 4. Obtener lista de archivos
      // ----------------------------
      std::vector<SourceFileDB> files;

      if (isSenior || isOwner) {
         // Senior u owner ven todos los archivos del proyecto
         files = projectRepositoryDB_.getAllFilesInProject(idProject);
      } else {
         // Usuario normal: solo archivos con permiso explícito
         files = projectRepositoryDB_.getFilesForUserInProject(idUser, idProject);
      }

      // ----------------------------
      // 5. Registrar commit de auditoría
      // ----------------------------
      projectRepositoryDB_.addCommit(
         idUser,
         std::nullopt,
         std::nullopt,
         true,
         "LIST_USER_FILES_IN_PROJECT",
         "User " + userEmail +
            " listed accessible files in project '" + projectName + "'"
      );

      return files;
   }

private:
   IProjectRepositoryDB &projectRepositoryDB_;
   IUserRepository      &userRepository_;
};
