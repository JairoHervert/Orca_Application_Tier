#pragma once
#include <string>
#include <sstream>
#include <stdexcept>
#include <optional>   // <-- importante para std::optional / std::nullopt

#include "../domain/repositories/IRepositoryStore.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"

class DecipherRepositoryUseCase {
public:
   explicit DecipherRepositoryUseCase(
      IRepositoryStore &repositoryStore,
      IUserRepository &userRepository,
      IProjectRepositoryDB &projectRepositoryDB)
      : repositoryStore_(repositoryStore),
        userRepository_(userRepository),
        projectRepositoryDB_(projectRepositoryDB) {}

   std::ostringstream execute(const std::string &repoName,
                              const std::string &userEmail,
                              const std::string &userPassword) {
      
      int userIdForCommit = -1;   // valor por defecto si no logramos obtener al usuario

      try {
         // Verificar existencia de los actores
         auto userOpt = userRepository_.findByEmail(userEmail);
         if (userOpt.has_value()) {
            userIdForCommit = userOpt->idUser;
         }

         if (!userOpt.has_value())
            throw std::runtime_error("User with email " + userEmail + " does not exist");

         if (!projectRepositoryDB_.existsRepoAlias(repoName))
            throw std::runtime_error("Project with name " + repoName + " does not exist in database");

         if (!repositoryStore_.findByNameInCiphers(repoName).has_value())
            throw std::runtime_error("Encrypted repository not found: " + repoName);

         if (!userRepository_.isValidPassword(userEmail, userPassword))
            throw std::runtime_error("Invalid password for user with email " + userEmail);

         // Verificar que el usuario este activo y verificado
         if (!userRepository_.isStatusActive(userEmail))
            throw std::runtime_error("User with email " + userEmail + " is not active");
         if (!userRepository_.isVerifiedUser(userEmail))
            throw std::runtime_error("User with email " + userEmail + " is not verified");

         // Solo el senior o usuarios asociados al proyecto pueden obtener el cifrado
         if (projectRepositoryDB_.existsUserInCipherProject(repoName, userOpt->idUser)) {

            // Obtener el .tar.enc como stream
            std::ostringstream cipherStream =
               repositoryStore_.getCipherAsStream(repoName + ".tar.enc");

            // Commit de éxito
            projectRepositoryDB_.addCommit(
               userIdForCommit,          // idUser
               std::nullopt,             // idFile (no hay archivo individual)
               std::nullopt,             // firma digital
               true,                     // isAccepted
               "DECIPHER_REPOSITORY",    // comando
               "User " + userEmail +
                  " retrieved encrypted repository " + repoName
            );

            return cipherStream;
         } else {
            throw std::runtime_error(
               "User with email " + userEmail +
               " does not have access to decipher project " + repoName
            );
         }

      } catch (const std::exception &e) {
         // Commit de fallo
         projectRepositoryDB_.addCommit(
            userIdForCommit,    // si nunca se encontró el usuario, será -1
            std::nullopt,
            std::nullopt,
            false,              // isAccepted
            "DECIPHER_REPOSITORY",
            std::string("Failed to retrieve encrypted repository ") +
               repoName + " for user " + userEmail + ": " + e.what()
         );
         throw;   // re-lanzar la excepción al llamador
      }
   }

private:
   IRepositoryStore &repositoryStore_;
   IUserRepository &userRepository_;
   IProjectRepositoryDB &projectRepositoryDB_;
};




// #pragma once
// #include <string>
// #include <sstream>
// #include <stdexcept>
// #include "../domain/repositories/IRepositoryStore.repository.hpp"
// #include "../domain/repositories/IProjectDB.repository.hpp"
// #include "../domain/repositories/IUser.repository.hpp"

// class DecipherRepositoryUseCase {
// public:
//    explicit DecipherRepositoryUseCase(
//       IRepositoryStore &repositoryStore,
//       IUserRepository &userRepository,
//       IProjectRepositoryDB &projectRepositoryDB)
//       : repositoryStore_(repositoryStore),
//          userRepository_(userRepository),
//          projectRepositoryDB_(projectRepositoryDB) {}

//    std::ostringstream execute(const std::string &repoName, const std::string &userEmail, const std::string &userPassword) {
      
//       // Verificar existencia de los actores
//       auto userOpt = userRepository_.findByEmail(userEmail);
      
//       if (!userOpt.has_value())
//          throw std::runtime_error("User with email " + userEmail + " does not exist");

//       if (!projectRepositoryDB_.existsRepoAlias(repoName))
//          throw std::runtime_error("Project with name " + repoName + " does not exist in database");

//       if (!repositoryStore_.findByNameInCiphers(repoName).has_value())
//          throw std::runtime_error("Encrypted repository not found: " + repoName);


//       if (!userRepository_.isValidPassword(userEmail, userPassword))
//          throw std::runtime_error("Invalid password for user with email " + userEmail);
      

//       // Verificar que el usuario este activo y verificado
//       if (!userRepository_.isStatusActive(userEmail))
//          throw std::runtime_error("User with email " + userEmail + " is not active");
//       if (!userRepository_.isVerifiedUser(userEmail))
//          throw std::runtime_error("User with email " + userEmail + " is not verified");

//       // Solo el senior o usuarios asociados al proyecto pueden descifrarlo
//       if (projectRepositoryDB_.existsUserInCipherProject(repoName, userOpt->idUser)) {
//          return repositoryStore_.getCipherAsStream(repoName + ".tar.enc");
//       } else {
//          throw std::runtime_error("User with email " + userEmail + " does not have access to decipher project " + repoName);
//       }
//    }

// private:
//    IRepositoryStore &repositoryStore_;
//    IUserRepository &userRepository_;
//    IProjectRepositoryDB &projectRepositoryDB_;
// };