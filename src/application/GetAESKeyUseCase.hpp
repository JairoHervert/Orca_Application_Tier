#pragma once
#include <string>
#include <stdexcept>
#include <optional>
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/entities/User.entity.hpp"
#include "../domain/entities/Repository.entity.hpp"

class GetAESKeyUseCase {
public:
   explicit GetAESKeyUseCase(
      IProjectRepositoryDB &projectRepositoryDB,
      IUserRepository      &userRepository
   )
      : projectRepositoryDB_(projectRepositoryDB),
        userRepository_(userRepository) {}

   std::string execute(
      const std::string &userEmail,
      const std::string &userPassword,
      const std::string &projectName,
      const std::string &projectAlias  // debe coincidir con lo guardado en repo_protect
   ) {
      int idUser = -1;   // para logging en commits incluso si algo falla

      try {
         // 1. Verificar usuario
         auto userOpt = userRepository_.findByEmail(userEmail);
         if (!userOpt.has_value()) {
            throw std::runtime_error("User not found: " + userEmail);
         }

         idUser = userOpt->idUser;

         if (!userRepository_.isValidPassword(userEmail, userPassword)) {
            throw std::runtime_error("Invalid password for user: " + userEmail);
         }

         if (!userRepository_.isVerifiedUser(userEmail)) {
            throw std::runtime_error("User is not verified: " + userEmail);
         }

         if (!userRepository_.isStatusActive(userEmail)) {
            throw std::runtime_error("User is not active: " + userEmail);
         }

         // 2. Verificar proyecto
         auto projectOpt = projectRepositoryDB_.findByName(projectName);
         if (!projectOpt.has_value()) {
            throw std::runtime_error("Project not found: " + projectName);
         }

         int idProject = projectOpt->idProject;

         // 3. Verificar que el usuario tenga entrada en repo_protect (acceso al repo cifrado)
         if (!projectRepositoryDB_.existsUserInCipherProject(projectAlias, idUser)) {
            throw std::runtime_error(
               "User does not have access to the encrypted project (alias): " +
               projectAlias
            );
         }

         // 4. Obtener la clave AES (en realidad rsa_aes: clave AES cifrada con RSA)
         std::string aesKeyEnc =
            projectRepositoryDB_.getAESKeyForRepo(idUser, idProject, projectAlias);

         if (aesKeyEnc.empty()) {
            throw std::runtime_error(
               "AES key not found for the given user/project/alias."
            );
         }

         // 5. Registrar commit de éxito
         projectRepositoryDB_.addCommit(
            idUser,
            std::nullopt,
            std::nullopt,
            true,
            "GET_AES_KEY",
            "User " + userEmail +
               " retrieved AES key for project '" + projectName +
               "' and alias '" + projectAlias + "'"
         );

         return aesKeyEnc;
      }
      catch (const std::exception &e) {
         // Registrar commit de error (si no se pudo resolver el usuario, idUser será -1)
         projectRepositoryDB_.addCommit(
            idUser,
            std::nullopt,
            std::nullopt,
            false,
            "GET_AES_KEY_ERROR",
            std::string("Error getting AES key for user '") + userEmail +
               "', project '" + projectName +
               "', alias '" + projectAlias +
               "': " + e.what()
         );

         // Volver a lanzar para que el controlador HTTP pueda devolver el error
         throw;
      }
   }

private:
   IProjectRepositoryDB &projectRepositoryDB_;
   IUserRepository      &userRepository_;
};
