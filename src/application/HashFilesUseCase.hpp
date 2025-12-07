#pragma once
#include <string>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <map>
#include <optional>

#include "../domain/repositories/IRepositoryStore.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/repositories/IPushRepoVerifyCrypto.repository.hpp"

class HashFilesUseCase {
public:
   explicit HashFilesUseCase(
            IRepositoryStore &repositoryStore,
            IProjectRepositoryDB &DBProjectRepository,
            IUserRepository &userRepository,
            IPushRepoCryptoRepository &cryptoRepo)
            : repositoryStore_(repositoryStore),
              DBProjectRepository_(DBProjectRepository),
              userRepository_(userRepository),
              cryptoRepo_(cryptoRepo) {}

   std::map<std::string, std::string> execute(
      const std::string &repoName,
      const std::string &userEmail,
      const std::string &userPassword) 
   {
      int userIdForCommit = -1;

      try {
         // Intentar obtener al usuario desde el inicio para tener su id para commits
         auto userOpt = userRepository_.findByEmail(userEmail);
         if (!userOpt.has_value()) {
            // aun así intentamos registrar un commit de fallo más abajo
            throw std::runtime_error("User not found: " + userEmail);
         }
         userIdForCommit = userOpt->idUser;

         auto repoOpt = DBProjectRepository_.findByName(repoName);
         if (!repoOpt.has_value()) {
            throw std::runtime_error("Repository not found: " + repoName);
         }

         // ver que el repositorio también exista en el storage
         auto repoStoreOpt = repositoryStore_.findByName(repoName);
         if (!repoStoreOpt.has_value()) {
            throw  std::runtime_error("Repository not found in storage: " + repoName);
         }

         // verificar la contraseña del usuario
         if (!userRepository_.isValidPassword(userEmail, userPassword)) {
            throw std::runtime_error("Invalid password for user: " + userEmail);
         }

         // Ver que el usuario esté verificado y activo
         if (!userRepository_.isVerifiedUser(userEmail)) {
            throw std::runtime_error("User is not verified: " + userEmail);
         }
         if (!userRepository_.isStatusActive(userEmail)) {
            throw std::runtime_error("User is not active: " + userEmail);
         }

         // Revisar que el usuario tenga acceso al proyecto o sea el owner o senior
         if (!DBProjectRepository_.existsUserInProject(repoOpt->idProject, userOpt->idUser)
             && repoOpt->ownerId != userOpt->idUser
             && !userRepository_.isSeniorUser(userEmail)) {
            throw std::runtime_error("User does not have access to the project: " + repoName);
         }

         // 4. Listar todos los archivos del repositorio
         std::vector<std::filesystem::path> files =
            repositoryStore_.listAllFiles(repoName);

         // 5. Calcular hash de cada archivo
         std::map<std::string, std::string> fileHashes;
         
         for (const auto& relativePath : files) {
            // Obtener ruta completa para calcular hash
            std::string fullPathStr =
               repositoryStore_.getFullPath(repoName, relativePath.string());
            
            // Calcular hash
            std::string hashB64 =
               cryptoRepo_.b64_hash_file_SHA256(fullPathStr);
            
            if (hashB64.empty()) {
               std::cerr << "Warning: Failed to hash file: "
                         << fullPathStr << std::endl;
               continue;
            }
            
            // Usar directamente la ruta relativa (a partir de la raíz del repo)
            fileHashes[relativePath.string()] = hashB64;
         }

         std::cout << "Hashed " << fileHashes.size()
                   << " files from repository " << repoName << std::endl;

         // Commit de éxito
         DBProjectRepository_.addCommit(
            userIdForCommit,
            std::nullopt,  // sin archivo concreto, es operación global
            std::nullopt,  // sin firma digital asociada
            true,          // isAccepted
            "HASH_REPOSITORY_FILES",
            "User " + userEmail + " hashed " +
               std::to_string(fileHashes.size()) +
               " files from repository " + repoName
         );

         return fileHashes;

      } catch (const std::exception &e) {
         // Commit de fallo (aunque el usuario no exista, usamos -1 como id)
         DBProjectRepository_.addCommit(
            userIdForCommit, // -1 si no se logró obtener el usuario
            std::nullopt,
            std::nullopt,
            false, // isAccepted
            "HASH_REPOSITORY_FILES",
            std::string("Failed to hash files from repository ") +
               repoName + " for user " + userEmail + ": " + e.what()
         );
         throw; // re-lanzar para que la capa superior maneje el error
      }
   }

private:
   IRepositoryStore &repositoryStore_;
   IProjectRepositoryDB &DBProjectRepository_;
   IUserRepository &userRepository_;
   IPushRepoCryptoRepository &cryptoRepo_;
};






// #pragma once
// #include <iostream>
// #include <string>
// #include <map>
// #include <stdexcept>
// #include <filesystem>
// #include "../domain/repositories/IPushRepoVerifyCrypto.repository.hpp"
// #include "../domain/repositories/IRepositoryStore.repository.hpp"
// #include "../domain/repositories/IProjectDB.repository.hpp"
// #include "../domain/repositories/IUser.repository.hpp"

// class HashFilesUseCase {
// public:
//    explicit HashFilesUseCase(
//             IRepositoryStore &repositoryStore,
//             IProjectRepositoryDB &DBProjectRepository,
//             IUserRepository &userRepository,
//             IPushRepoCryptoRepository &cryptoRepo)
//             : repositoryStore_(repositoryStore),
//                DBProjectRepository_(DBProjectRepository),
//                userRepository_(userRepository),
//                cryptoRepo_(cryptoRepo) {}

//    std::map<std::string, std::string> execute(const std::string &repoName, const std::string &userEmail, const std::string &userPassword) {
//       // 1-3. Validaciones...
//       auto userOpt = userRepository_.findByEmail(userEmail);
//       auto repoOpt = DBProjectRepository_.findByName(repoName);
//       if (!userOpt.has_value()) {
//          throw std::runtime_error("User not found: " + userEmail);
//       }
//       if (!repoOpt.has_value()) {
//          throw std::runtime_error("Repository not found: " + repoName);
//       }

//       // ver que el repositorio tambien exista en el storage
//       auto repoStoreOpt = repositoryStore_.findByName(repoName);
//       if (!repoStoreOpt.has_value()) {
//          throw  std::runtime_error("Repository not found in storage: " + repoName);
//       }

//       // verificar la contraseña del usuario
//       if (!userRepository_.isValidPassword(userEmail, userPassword)) {
//          throw std::runtime_error("Invalid password for user: " + userEmail);
//       }

//       // Ver que el usuario este verificado y activo
//       if (!userRepository_.isVerifiedUser(userEmail)) {
//          throw std::runtime_error("User is not verified: " + userEmail);
//       }
//       if (!userRepository_.isStatusActive(userEmail)) {
//          throw std::runtime_error("User is not active: " + userEmail);
//       }

//       // Revisar que el usuario tenga acceso al proyecto o sea el owner o senior
//       if (!DBProjectRepository_.existsUserInProject(repoOpt->idProject, userOpt->idUser) && repoOpt->ownerId != userOpt->idUser && !userRepository_.isSeniorUser(userEmail)) {
//          throw std::runtime_error("User does not have access to the project: " + repoName);
//       }

      
      
//       // 4. Listar todos los archivos del repositorio
//       std::vector<std::filesystem::path> files = repositoryStore_.listAllFiles(repoName);

//       // 5. Calcular hash de cada archivo
//       std::map<std::string, std::string> fileHashes;
      
//       for (const auto& relativePath : files) {
//          // Obtener ruta completa para calcular hash
//          std::string fullPathStr = repositoryStore_.getFullPath(repoName, relativePath.string());
         
//          // Calcular hash
//          std::string hashB64 = cryptoRepo_.b64_hash_file_SHA256(fullPathStr);
         
//          if (hashB64.empty()) {
//             std::cerr << "Warning: Failed to hash file: " << fullPathStr << std::endl;
//             continue;
//          }
         
//          // Usar directamente la ruta relativa (a partir de la raíz del repo) : carpeta/archivo.txt
//          fileHashes[relativePath.string()] = hashB64;
//       }

//       std::cout << "Hashed " << fileHashes.size() << " files from repository " << repoName << std::endl;
      
//       return fileHashes;
//    }

// private:
//    IRepositoryStore &repositoryStore_;
//    IProjectRepositoryDB &DBProjectRepository_;
//    IUserRepository &userRepository_;
//    IPushRepoCryptoRepository &cryptoRepo_;
// };