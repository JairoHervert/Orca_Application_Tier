#pragma once
#include <iostream>
#include <string>
#include <map>
#include <stdexcept>
#include <filesystem>

#include "../domain/repositories/IPushRepoVerifyCrypto.repository.hpp"
#include "../domain/repositories/IRepositoryStore.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/entities/SourceFileDB.entity.hpp"
#include "../domain/entities/User.entity.hpp"
#include "../domain/entities/Repository.entity.hpp"

class PushVerifyUseCase {
public:
   explicit PushVerifyUseCase(
      IRepositoryStore &repositoryStore,
      IProjectRepositoryDB &DBProjectRepository,
      IUserRepository &userRepository,
      IPushRepoCryptoRepository &cryptoRepo)
      : repositoryStore_(repositoryStore),
        DBProjectRepository_(DBProjectRepository),
        userRepository_(userRepository),
        cryptoRepo_(cryptoRepo) {}

   bool execute(const std::string &userEmail,
                const std::string &userPassword,
                const std::string &repoName,
                const std::string &tarFilename,  // p. ej. "cambios.tar"
                const std::string &tarContent,   // bytes del .tar
                const std::map<std::string, std::string> &fileSignatures) {

      std::optional<User>          userOpt;
      std::optional<Repository>    repoOpt;
      std::string                  identifier;

      try {
         // ------------------------------------------------------------------
         // 0. Resolver usuario / repo y hacer commit de inicio
         // ------------------------------------------------------------------
         userOpt = userRepository_.findByEmail(userEmail);
         repoOpt = DBProjectRepository_.findByName(repoName);

         if (!userOpt.has_value())
            throw std::runtime_error("User not found: " + userEmail);

         if (!repoOpt.has_value())
            throw std::runtime_error("Repository not found: " + repoName);

         // Ver que el repositorio tambien exista en el storage
         auto repoStoreOpt = repositoryStore_.findByName(repoName);
         if (!repoStoreOpt.has_value())
            throw std::runtime_error("Repository not found in storage: " + repoName);

         // verificar la contraseña del usuario
         if (!userRepository_.isValidPassword(userEmail, userPassword))
            throw std::runtime_error("Invalid password for user: " + userEmail);

         // ver que el usuario este verificado y activo
         if (!userRepository_.isVerifiedUser(userEmail))
            throw std::runtime_error("User is not verified: " + userEmail);

         if (!userRepository_.isStatusActive(userEmail))
            throw std::runtime_error("User is not active: " + userEmail);

         // Revisar que el usuario tenga acceso al proyecto o sea el owner o senior
         if (!DBProjectRepository_.existsUserInProject(repoOpt->idProject, userOpt->idUser) &&
             repoOpt->ownerId != userOpt->idUser &&
             !userRepository_.isSeniorUser(userEmail)) {
            throw std::runtime_error("User does not have access to the project: " + repoName);
         }

         // Commit de inicio de operación
         DBProjectRepository_.addCommit(
            userOpt->idUser,
            std::nullopt,                      // sin archivo asociado
            std::nullopt,                      // sin firma
            true,                              // es un commit informativo exitoso
            "PUSH_VERIFY_START",
            "Starting push & verify on repository " + repoName
         );

         // ------------------------------------------------------------------
         // 1. Guardar y extraer el tar en workspace
         // ------------------------------------------------------------------
         std::filesystem::path tempTarPath = repositoryStore_.saveTarToWorkspace(tarContent, tarFilename);
         std::filesystem::path extractedFolderPath = repositoryStore_.extractTarInWorkspace(tarFilename);

         std::cout << "Temporary tar path: " << tempTarPath.string() << std::endl;
         std::cout << "Extracted folder path: " << extractedFolderPath.string() << std::endl;

         identifier = std::filesystem::path(tarFilename).stem().string(); // p.ej. "cambios"

         // ------------------------------------------------------------------
         // 2. Verificar firmas de cada archivo
         // ------------------------------------------------------------------
         for (const auto& [relativePath, signatureB64] : fileSignatures) {

            // Validar que el usuario tiene permiso sobre el archivo:
            //  - o bien tiene permiso explícito en filepermissions,
            //  - o es owner del proyecto,
            //  - o es senior.
            auto filePermOpt = DBProjectRepository_.existsFileInProject(relativePath, repoOpt->idProject);

            bool hasPermFile = false;
            if (filePermOpt.has_value()) {
               // Si el archivo ya está registrado, verificamos en filepermissions
               hasPermFile = DBProjectRepository_.existsUserFilePermission(
                  userOpt->idUser,
                  filePermOpt->idsourcefile
               );
            }

            bool hasPerm = hasPermFile ||
                           (repoOpt->ownerId == userOpt->idUser) ||
                           userRepository_.isSeniorUser(userEmail);

            if (!hasPerm) {
               std::cerr << "User " << userOpt->idUser
                         << " does not have permission for file: " << relativePath << std::endl;

               repositoryStore_.cleanWorkspace(identifier);
               throw std::runtime_error("User does not have permission for file: " + relativePath);
            }

            std::filesystem::path fullFilePath = extractedFolderPath / relativePath;

            // Calcular hash SHA256 en base64
            std::string calculatedHashB64 = cryptoRepo_.b64_hash_file_SHA256(fullFilePath.string());

            // Imprimir para depuración
            std::cout << "Verifying file: " << relativePath << std::endl;
            std::cout << " - Calculated Hash: " << calculatedHashB64 << std::endl;
            std::cout << " - Provided Signature: " << signatureB64 << std::endl;

            // consultar la clave pública del usuario
            std::string publicKeyB64 = userOpt->publicKeyECDSA;
            std::cout << " - User Public Key (ECDSA Base64): " << publicKeyB64 << std::endl;

            // Verificar la firma
            bool signatureValid = cryptoRepo_.verify_signature_ecdsa_p256(
               fullFilePath.string(),
               signatureB64,
               publicKeyB64
            );

            if (!signatureValid) {
               std::cerr << "Signature verification failed for file: " << relativePath << std::endl;

               repositoryStore_.cleanWorkspace(identifier);
               throw std::runtime_error("Signature verification failed for file: " + relativePath);
            } else {
               std::cout << "Signature valid for file: " << relativePath << std::endl;
            }

            std::cout << std::endl;
         }

         // ------------------------------------------------------------------
         // 3. Sustituir / agregar archivos en el repo real
         // ------------------------------------------------------------------
         for (const auto& [relativePath, signatureB64] : fileSignatures) {

            // Validación de seguridad (dominio)
            std::filesystem::path rel = std::filesystem::path(relativePath).lexically_normal();
            if (rel.is_absolute() || relativePath.find("..") != std::string::npos) {
               repositoryStore_.cleanWorkspace(identifier);
               throw std::runtime_error("Invalid relative path in push: " + relativePath);
            }

            // Actualizar el archivo desde workspace al repo real
            repositoryStore_.updateFileFromWorkspace(repoName, extractedFolderPath, rel);
         }

         // ------------------------------------------------------------------
         // 4. Registrar archivos, permisos y commits por archivo
         // ------------------------------------------------------------------
         for (const auto& [relativePath, signatureB64] : fileSignatures) {

            // 4.1 Buscar / insertar en sourcefiles
            auto fileOpt = DBProjectRepository_.existsFileInProject(relativePath, repoOpt->idProject);

            if (!fileOpt.has_value()) {
               // No existe -> lo agregamos al proyecto
               bool fileAdded = DBProjectRepository_.addFileToProject(relativePath, repoOpt->idProject);
               if (!fileAdded) {
                  std::cerr << "Failed to add file to project in DB: " << relativePath << std::endl;
                  // No lanzamos excepción para no dejar inconsistente toda la operación,
                  // pero podrías decidir lo contrario.
                  continue;
               }

               // Volver a consultarlo para obtener el idsourcefile
               fileOpt = DBProjectRepository_.existsFileInProject(relativePath, repoOpt->idProject);
               if (!fileOpt.has_value()) {
                  std::cerr << "File was inserted but cannot be found: " << relativePath << std::endl;
                  continue;
               }
            }

            SourceFileDB fileDB = *fileOpt;
            int idFile          = fileDB.idsourcefile;
            int idUser          = userOpt->idUser;

            // 4.2 Asignar permisos al usuario que hizo el push, si no los tiene
            bool hasPerm = DBProjectRepository_.existsUserFilePermission(idUser, idFile);
            if (!hasPerm) {
               bool permAdded = DBProjectRepository_.addUserFilePermission(idUser, idFile);
               if (!permAdded) {
                  std::cerr << "Failed to add user-file permission for user "
                            << idUser << " and file " << idFile << std::endl;
                  // seguimos, pero sin commit de este archivo
                  continue;
               }
            }

            // 4.3 Hacer commit por este archivo modificado
            DBProjectRepository_.addCommit(
               idUser,
               idFile,                 // archivo asociado
               signatureB64,           // podemos guardar la firma del archivo
               true,                   // operación aceptada para este archivo
               "PUSH_VERIFY_FILE",
               "File " + relativePath + " updated in repository " + repoName
            );
         }

         // ------------------------------------------------------------------
         // 5. Limpiar workspace
         // ------------------------------------------------------------------
         repositoryStore_.cleanWorkspace(identifier);

         // ------------------------------------------------------------------
         // 6. Commit de finalización exitosa
         // ------------------------------------------------------------------
         DBProjectRepository_.addCommit(
            userOpt->idUser,
            std::nullopt,
            std::nullopt,
            true,
            "PUSH_VERIFY_END",
            "Push & verify successfully finished on repository " + repoName
         );

         return true;
      }
      catch (const std::exception &e) {

         // Limpiar workspace si tenemos identificador
         if (!identifier.empty()) {
            repositoryStore_.cleanWorkspace(identifier);
         }

         // Commit de fallo global
         int userId = (userOpt.has_value() ? userOpt->idUser : -1);
         DBProjectRepository_.addCommit(
            userId,
            std::nullopt,
            std::nullopt,
            false,  // fallo
            "PUSH_VERIFY_ERROR",
            std::string("Push & verify failed on repository ") + repoName +
               " : " + e.what()
         );

         throw;  // propagar el error a la capa superior
      }
   }

private:
   IRepositoryStore        &repositoryStore_;
   IProjectRepositoryDB    &DBProjectRepository_;
   IUserRepository         &userRepository_;
   IPushRepoCryptoRepository &cryptoRepo_;
};








// #pragma once
// #include <iostream>
// #include <string>
// #include <map>
// #include <stdexcept>
// #include "../domain/repositories/IPushRepoVerifyCrypto.repository.hpp"
// #include "../domain/repositories/IRepositoryStore.repository.hpp"
// #include "../domain/repositories/IProjectDB.repository.hpp"
// #include "../domain/repositories/IUser.repository.hpp"

// class PushVerifyUseCase {
// public:
//    explicit PushVerifyUseCase(
//       IRepositoryStore &repositoryStore,
//       IProjectRepositoryDB &DBProjectRepository,
//       IUserRepository &userRepository,
//       IPushRepoCryptoRepository &cryptoRepo)
//       : repositoryStore_(repositoryStore),
//         DBProjectRepository_(DBProjectRepository),
//         userRepository_(userRepository),
//         cryptoRepo_(cryptoRepo) {}

//    bool execute(const std::string &userEmail,
//                 const std::string &userPassword,
//                 const std::string &repoName,
//                 const std::string &tarFilename,  // p. ej. "cambios.tar"
//                 const std::string &tarContent,   // bytes del .tar
//                 const std::map<std::string, std::string> &fileSignatures) {

//       // 1. Validar usuario y repo con userRepository_ y DBProjectRepository_
//       auto userOpt = userRepository_.findByEmail(userEmail);
//       auto repoOpt = DBProjectRepository_.findByName(repoName);
      
//       if (!userOpt.has_value()) {
//          throw std::runtime_error("User not found: " + userEmail);
//          return false;
//       }

//       if (!repoOpt.has_value()) {
//          throw std::runtime_error("Repository not found: " + repoName);         
//          return false;
//       }

//       // ver que el repositorio tambien exista en el storage
//       auto repoStoreOpt = repositoryStore_.findByName(repoName);
//       if (!repoStoreOpt.has_value()) {
//          throw  std::runtime_error("Repository not found in storage: " + repoName);
//       }

//       // verificar la contraseña del usuario
//       if (!userRepository_.isValidPassword(userEmail, userPassword)) {
//          throw std::runtime_error("Invalid password for user: " + userEmail);
//          return false;
//       }

//       // ver que el usuario este verificado y activo
//       if (!userRepository_.isVerifiedUser(userEmail)) {
//          throw std::runtime_error("User is not verified: " + userEmail);
//          return false;
//       }
//       if (!userRepository_.isStatusActive(userEmail)) {
//          throw std::runtime_error("User is not active: " + userEmail);
//          return false;
//       }

//       // Revisar que el usuario tenga acceso al proyecto o sea el owner o senior
//       if (!DBProjectRepository_.existsUserInProject(repoOpt->idProject, userOpt->idUser) && repoOpt->ownerId != userOpt->idUser && !userRepository_.isSeniorUser(userEmail)) {
//          throw std::runtime_error("User does not have access to the project: " + repoName);
//          return false;
//       }


//       // 2. Guardar el tar en disco. Usa la carpeta workspace del repositoryStore_
//       std::filesystem::path tempTarPath = repositoryStore_.saveTarToWorkspace(tarContent, tarFilename);

//       // 3. Desempaquetar el tar en una carpeta temporal (repositoryStore_)
//       std::filesystem::path extractedFolderPath = repositoryStore_.extractTarInWorkspace(tarFilename);

//       std::cout << "Temporary tar path: " << tempTarPath.string() << std::endl;
//       std::cout << "Extracted folder path: " << extractedFolderPath.string() << std::endl;



//       // 4. Calcular el hash de cada archivo
//       for (const auto& [relativePath, signatureB64] : fileSignatures) {

//          // Verificar que el usuario tiene permiso sobre el archivo (tambien puede modificar si es owner o senior)
//          bool hasPerm = DBProjectRepository_.existsUserFilePermission(userOpt->idUser, repoOpt->idProject) ||
//                         repoOpt->ownerId == userOpt->idUser ||
//                         userRepository_.isSeniorUser(userEmail);
                        
//          if (!hasPerm) {
//             std::cerr << "User " << userOpt->idUser << " does not have permission for file: " << relativePath << std::endl;
//             // Limpiar archivos temporales del workspace
//             std::string identifier = std::filesystem::path(tarFilename).stem().string();
//             repositoryStore_.cleanWorkspace(identifier);
//             throw std::runtime_error("User does not have permission for file: " + relativePath);
//             return false;
//          }

//          std::filesystem::path fullFilePath = extractedFolderPath / relativePath;

//          // Calcular hash SHA256 en base64
//          std::string calculatedHashB64 = cryptoRepo_.b64_hash_file_SHA256(fullFilePath.string());

//          // Imprimir para depuración
//          std::cout << "Verifying file: " << relativePath << std::endl;
//          std::cout << " - Calculated Hash: " << calculatedHashB64 << std::endl;
//          std::cout << " - Provided Signature: " << signatureB64 << std::endl;

//          // consultar la clave pública del usuario
//          std::string publicKeyB64 = userOpt->publicKeyECDSA;
//          std::cout << " - User Public Key (ECDSA Base64): " << publicKeyB64 << std::endl;

//          // Verificar la firma
//          bool signatureValid = cryptoRepo_.verify_signature_ecdsa_p256(
//             fullFilePath.string(),
//             signatureB64,
//             publicKeyB64
//          );

//          if (!signatureValid) {
//             std::cerr << "Signature verification failed for file: " << relativePath << std::endl;

//             // Limpiar archivos temporales del workspace
//             std::string identifier = std::filesystem::path(tarFilename).stem().string();
//             repositoryStore_.cleanWorkspace(identifier);

//             // Mostrar error y abortar
//             throw std::runtime_error("Signature verification failed for file: " + relativePath);
//             return false;
//          } else {
//             std::cout << "Signature valid for file: " << relativePath << std::endl;
//          }

//          std::cout << std::endl;
//       }
      

//       // Si la verificación es exitosa para todos los archivos, proceder con el push
//       // === 5. TODAS LAS FIRMAS OK → SUSTITUIR / AGREGAR ARCHIVOS EN EL REPO REAL ===
//       for (const auto& [relativePath, signatureB64] : fileSignatures) {

//          // Validación de seguridad (dominio)
//          std::filesystem::path rel = std::filesystem::path(relativePath).lexically_normal();
//          if (rel.is_absolute() || relativePath.find("..") != std::string::npos) {
//             std::string identifier = std::filesystem::path(tarFilename).stem().string();
//             repositoryStore_.cleanWorkspace(identifier);
//             throw std::runtime_error("Invalid relative path in push: " + relativePath);
//          }

//          // Delegar la actualización del archivo al repositorio de almacenamiento
//          repositoryStore_.updateFileFromWorkspace(repoName, extractedFolderPath, rel);
//       }

//       // 6. eliminar archivos temporales del workspace
//       std::string identifier = std::filesystem::path(tarFilename).stem().string(); // "cambios"
//       repositoryStore_.cleanWorkspace(identifier);


//       // 7. Registrar archivos y permisos en la base de datos
//       // aqui si no esta en la tabla "sourcefiles" agregarlo y tambien a "filepermisions" ya que si es nuevo archivo el usuario que hizo el push debe tener permisos automaticamente
//       for (const auto& [relativePath, signatureB64] : fileSignatures) {

//          // 7.1 Buscar el archivo en la tabla sourcefiles
//          auto fileOpt = DBProjectRepository_.existsFileInProject(relativePath, repoOpt->idProject);

//          if (!fileOpt.has_value()) {
//             // No existe -> lo agregamos al proyecto
//             bool fileAdded = DBProjectRepository_.addFileToProject(relativePath, repoOpt->idProject);
//             if (!fileAdded) {
//                std::cerr << "Failed to add file to project in DB: " << relativePath << std::endl;
//                continue;
//             }

//             // Volver a consultarlo para obtener el idsourcefile
//             fileOpt = DBProjectRepository_.existsFileInProject(relativePath, repoOpt->idProject);
//             if (!fileOpt.has_value()) {
//                std::cerr << "File was inserted but cannot be found: " << relativePath << std::endl;
//                continue;
//             }
//          }

//          SourceFileDB fileDB = *fileOpt;
//          int idFile = fileDB.idsourcefile;

//          // 7.2 Asignar permisos al usuario que hizo el push, si no los tiene
//          int idUser = userOpt->idUser;   // ajuste el nombre del campo si es distinto

//          bool hasPerm = DBProjectRepository_.existsUserFilePermission(idUser, idFile);
//          if (!hasPerm) {
//             bool permAdded = DBProjectRepository_.addUserFilePermission(idUser, idFile);
//             if (!permAdded) {
//                std::cerr << "Failed to add user-file permission for user " << idUser << " and file " << idFile << std::endl;
//                // Puede decidir si continúa o lanza excepción
//             }
//          }
//       }

//       return true; // o false, según el resultado real
//    }

// private:
//    IRepositoryStore &repositoryStore_;
//    IProjectRepositoryDB &DBProjectRepository_;
//    IUserRepository &userRepository_;
//    IPushRepoCryptoRepository &cryptoRepo_;
// };
