#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>

#include "../domain/repositories/IPushRepoVerifyCrypto.repository.hpp"
#include "../domain/repositories/IRepositoryStore.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/entities/SourceFileDB.entity.hpp"
#include "../domain/entities/User.entity.hpp"
#include "../domain/entities/Repository.entity.hpp"
#include "../domain/entities/PushOperation.entity.hpp"

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
                const std::vector<PushOperation> &operations) {

      std::optional<User>       userOpt;
      std::optional<Repository> repoOpt;
      std::string               identifier;

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

         auto repoStoreOpt = repositoryStore_.findByName(repoName);
         if (!repoStoreOpt.has_value())
            throw std::runtime_error("Repository not found in storage: " + repoName);

         if (!userRepository_.isValidPassword(userEmail, userPassword))
            throw std::runtime_error("Invalid password for user: " + userEmail);

         if (!userRepository_.isVerifiedUser(userEmail))
            throw std::runtime_error("User is not verified: " + userEmail);

         if (!userRepository_.isStatusActive(userEmail))
            throw std::runtime_error("User is not active: " + userEmail);

         if (!DBProjectRepository_.existsUserInProject(repoOpt->idProject, userOpt->idUser) &&
             repoOpt->ownerId != userOpt->idUser &&
             !userRepository_.isSeniorUser(userEmail)) {
            throw std::runtime_error("User does not have access to the project: " + repoName);
         }

         DBProjectRepository_.addCommit(
            userOpt->idUser,
            std::nullopt,
            std::nullopt,
            true,
            "PUSH_VERIFY_START",
            "Starting push & verify on repository " + repoName
         );

         // ------------------------------------------------------------------
         // 1. Guardar y extraer el tar en workspace (solo necesario para updates)
         // ------------------------------------------------------------------
         std::filesystem::path extractedFolderPath;
         if (!tarContent.empty()) {
            std::filesystem::path tempTarPath = repositoryStore_.saveTarToWorkspace(tarContent, tarFilename);
            extractedFolderPath = repositoryStore_.extractTarInWorkspace(tarFilename);

            std::cout << "Temporary tar path: " << tempTarPath.string() << std::endl;
            std::cout << "Extracted folder path: " << extractedFolderPath.string() << std::endl;
         }

         identifier = std::filesystem::path(tarFilename).stem().string(); // p.ej. "cambios"

         // ------------------------------------------------------------------
         // 2. Verificar operaciones (permisos + firmas)
         // ------------------------------------------------------------------
         for (const auto &op : operations) {
            std::cout << std::endl;
            const std::string &relativePath = op.path;
            const std::string &signatureB64 = op.signature;

            // 2.1 Permisos por archivo
            auto fileOpt = DBProjectRepository_.existsFileInProject(relativePath, repoOpt->idProject);

            bool hasPermFile = false;
            if (fileOpt.has_value()) {
               hasPermFile = DBProjectRepository_.existsUserFilePermission(
                  userOpt->idUser,
                  fileOpt->idsourcefile
               );
            }

            bool hasPerm = hasPermFile ||
                           (repoOpt->ownerId == userOpt->idUser) ||
                           userRepository_.isSeniorUser(userEmail);

            if (!hasPerm) {
               // Si el archivo no existe y es un update (archivo nuevo), podrías
               // opcionalmente permitirlo si el usuario pertenece al proyecto:
               if (op.op == "update" && !fileOpt.has_value()) {
                  hasPerm = DBProjectRepository_.existsUserInProject(
                     repoOpt->idProject, userOpt->idUser
                  ) || (repoOpt->ownerId == userOpt->idUser) ||
                       userRepository_.isSeniorUser(userEmail);
               }
            }

            if (!hasPerm) {
               repositoryStore_.cleanWorkspace(identifier);
               throw std::runtime_error(
                  "User does not have permission for file: " + relativePath
               );
            }

            // 2.2 Verificación criptográfica según el tipo de operación
            std::string publicKeyB64 = userOpt->publicKeyECDSA;

            if (op.op == "update") {
               if (tarContent.empty()) {
                  repositoryStore_.cleanWorkspace(identifier);
                  throw std::runtime_error("No tar content provided for update operation");
               }

               std::filesystem::path fullFilePath = extractedFolderPath / relativePath;

               std::string calculatedHashB64 =
                  cryptoRepo_.b64_hash_file_SHA256(fullFilePath.string());

               std::cout << "Verifying UPDATE for file: " << relativePath << "\n";
               std::cout << " - Hash(from file): " << calculatedHashB64 << "\n";
               std::cout << " - Signature: " << signatureB64 << "\n";

               bool signatureValid = cryptoRepo_.verify_signature_ecdsa_p256(
                  fullFilePath.string(),  // el helper re-calcula el hash internamente
                  signatureB64,
                  publicKeyB64
               );

               if (!signatureValid) {
                  repositoryStore_.cleanWorkspace(identifier);
                  throw std::runtime_error(
                     "Signature verification failed for updated file: " + relativePath
                  );
               }
            }
            else if (op.op == "delete") {
               // Para delete, verificamos la firma sobre el hash del archivo
               // tal como existe actualmente en el servidor.
               std::string fullPathStr =
                  repositoryStore_.getFullPath(repoName, relativePath);

               std::string serverHashB64 =
                  cryptoRepo_.b64_hash_file_SHA256(fullPathStr);

               std::cout << "Verifying DELETE for file: " << relativePath << "\n";
               std::cout << " - Hash(server file): " << serverHashB64 << "\n";
               std::cout << " - Signature: " << signatureB64 << "\n";

               // Aquí suponemos que en el cliente firmaste el hash (string)
               bool signatureValid =
                  cryptoRepo_.verify_signature_ecdsa_p256_over_string(
                     serverHashB64,
                     signatureB64,
                     publicKeyB64
                  );

               if (!signatureValid) {
                  repositoryStore_.cleanWorkspace(identifier);
                  throw std::runtime_error(
                     "Signature verification failed for delete of file: " + relativePath
                  );
               }
            }
            else {
               repositoryStore_.cleanWorkspace(identifier);
               throw std::runtime_error("Unknown operation type: " + op.op);
            }
         }

         // ------------------------------------------------------------------
         // 3. Aplicar operaciones en el repositorio real (update + delete)
         // ------------------------------------------------------------------
         for (const auto &op : operations) {
            std::cout << std::endl;
            std::filesystem::path rel = std::filesystem::path(op.path).lexically_normal();
            if (rel.is_absolute() || op.path.find("..") != std::string::npos) {
               repositoryStore_.cleanWorkspace(identifier);
               throw std::runtime_error("Invalid relative path in push: " + op.path);
            }

            if (op.op == "update") {
               repositoryStore_.updateFileFromWorkspace(repoName, extractedFolderPath, rel);
            }
            else if (op.op == "delete") {
               bool removedFs = repositoryStore_.deleteFile(repoName, rel);
               if (!removedFs) {
                  std::cerr << "Failed to delete file in filesystem: " << op.path << std::endl;
                  // aquí decides si lanzar excepción o solo registrar
               }

               // Además, eliminar en la BDD
               // bool removedDb = DBProjectRepository_.deleteFileFromProject(
               //    op.path,      // ruta relativa tal como la usas en sourcefiles.route
               //    repoOpt->idProject
               // );
               // if (!removedDb) {
               //    std::cerr << "Failed to delete file in DB for path: " << op.path << std::endl;
               //    // igual, puedes decidir lanzar o solo loggear
               // }
            }
         }


         // ------------------------------------------------------------------
         // 4. Registrar archivos, permisos y commits por archivo
         // ------------------------------------------------------------------
         for (const auto &op : operations) {
            const std::string &relativePath = op.path;
            const std::string &signatureB64 = op.signature;
            int  idUser                     = userOpt->idUser;

            // DELETE: el archivo puede desaparecer de sourcefiles si quieres
            if (op.op == "delete") {
               auto fileOpt = DBProjectRepository_.existsFileInProject(relativePath, repoOpt->idProject);
               if (fileOpt.has_value()) {
                  int idFile = fileOpt->idsourcefile;

                  // Podrías:
                  //  - Marcarlo como borrado
                  //  - O borrarlo de sourcefiles y filepermissions
                  // Para mantener simple, dejamos solo el commit:
                  DBProjectRepository_.addCommit(
                     idUser,
                     idFile,
                     signatureB64,
                     true,
                     "PUSH_VERIFY_DELETE",
                     "File " + relativePath + " deleted from repository " + repoName
                  );
               } else {
                  // Si no está en BD, solo registramos el commit sin idFile
                  DBProjectRepository_.addCommit(
                     idUser,
                     std::nullopt,
                     signatureB64,
                     true,
                     "PUSH_VERIFY_DELETE",
                     "File " + relativePath + " deleted (not found in DB) from repository " + repoName
                  );
               }
               continue;
            }

            // UPDATE:
            if (op.op == "update") {
               auto fileOpt = DBProjectRepository_.existsFileInProject(relativePath, repoOpt->idProject);

               if (!fileOpt.has_value()) {
                  bool fileAdded = DBProjectRepository_.addFileToProject(relativePath, repoOpt->idProject);
                  if (!fileAdded) {
                     std::cerr << "Failed to add file to project in DB: "
                               << relativePath << std::endl;
                     continue;
                  }
                  fileOpt = DBProjectRepository_.existsFileInProject(relativePath, repoOpt->idProject);
                  if (!fileOpt.has_value()) {
                     std::cerr << "File was inserted but cannot be found: "
                               << relativePath << std::endl;
                     continue;
                  }
               }

               SourceFileDB fileDB = *fileOpt;
               int          idFile = fileDB.idsourcefile;

               bool hasPerm = DBProjectRepository_.existsUserFilePermission(idUser, idFile);
               if (!hasPerm) {
                  bool permAdded = DBProjectRepository_.addUserFilePermission(idUser, idFile);
                  if (!permAdded) {
                     std::cerr << "Failed to add user-file permission for user "
                               << idUser << " and file " << idFile << std::endl;
                     continue;
                  }
               }

               DBProjectRepository_.addCommit(
                  idUser,
                  idFile,
                  signatureB64,
                  true,
                  "PUSH_VERIFY_FILE",
                  "File " + relativePath + " updated in repository " + repoName
               );
            }
         }

         // ------------------------------------------------------------------
         // 5. Limpiar workspace
         // ------------------------------------------------------------------
         if (!identifier.empty()) {
            repositoryStore_.cleanWorkspace(identifier);
         }

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

         if (!identifier.empty()) {
            repositoryStore_.cleanWorkspace(identifier);
         }

         int userId = (userOpt.has_value() ? userOpt->idUser : -1);
         DBProjectRepository_.addCommit(
            userId,
            std::nullopt,
            std::nullopt,
            false,
            "PUSH_VERIFY_ERROR",
            std::string("Push & verify failed on repository ") + repoName +
               " : " + e.what()
         );

         throw;
      }
   }

private:
   IRepositoryStore          &repositoryStore_;
   IProjectRepositoryDB      &DBProjectRepository_;
   IUserRepository           &userRepository_;
   IPushRepoCryptoRepository &cryptoRepo_;
};
