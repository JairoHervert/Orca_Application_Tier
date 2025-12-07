#pragma once
#include <iostream>
#include <string>
#include <map>
#include <stdexcept>
#include "../domain/repositories/IPushRepoVerifyCrypto.repository.hpp"
#include "../domain/repositories/IRepositoryStore.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"

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

      // 1. Validar usuario y repo con userRepository_ y DBProjectRepository_
      auto userOpt = userRepository_.findByEmail(userEmail);
      auto repoOpt = DBProjectRepository_.findByName(repoName);
      
      if (!userOpt.has_value()) {
         std::cerr << "User not found: " << userEmail << std::endl;
         return false;
      }

      if (!repoOpt.has_value()) {
         std::cerr << "Repository not found: " << repoName << std::endl;
         return false;
      }

      // 2. Guardar el tar en disco. Usa la carpeta workspace del repositoryStore_
      std::filesystem::path tempTarPath = repositoryStore_.saveTarToWorkspace(tarContent, tarFilename);

      // 3. Desempaquetar el tar en una carpeta temporal (repositoryStore_)
      std::filesystem::path extractedFolderPath = repositoryStore_.extractTarInWorkspace(tarFilename);

      std::cout << "Temporary tar path: " << tempTarPath.string() << std::endl;
      std::cout << "Extracted folder path: " << extractedFolderPath.string() << std::endl;



      // 4. Calcular el hash de cada archivo
      for (const auto& [relativePath, signatureB64] : fileSignatures) {
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

            // Limpiar archivos temporales del workspace
            std::string identifier = std::filesystem::path(tarFilename).stem().string();
            repositoryStore_.cleanWorkspace(identifier);

            // Mostrar error y abortar
            throw std::runtime_error("Signature verification failed for file: " + relativePath);
            return false;
         } else {
            std::cout << "Signature valid for file: " << relativePath << std::endl;
         }

         std::cout << std::endl;
      }
      

      // Si la verificación es exitosa para todos los archivos, proceder con el push
      // === 5. TODAS LAS FIRMAS OK → SUSTITUIR / AGREGAR ARCHIVOS EN EL REPO REAL ===
      for (const auto& [relativePath, signatureB64] : fileSignatures) {

         // Validación de seguridad (dominio)
         std::filesystem::path rel = std::filesystem::path(relativePath).lexically_normal();
         if (rel.is_absolute() || relativePath.find("..") != std::string::npos) {
            std::string identifier = std::filesystem::path(tarFilename).stem().string();
            repositoryStore_.cleanWorkspace(identifier);
            throw std::runtime_error("Invalid relative path in push: " + relativePath);
         }

         // Delegar la actualización del archivo al repositorio de almacenamiento
         repositoryStore_.updateFileFromWorkspace(repoName, extractedFolderPath, rel);
      }

      // 6. eliminar archivos temporales del workspace
      std::string identifier = std::filesystem::path(tarFilename).stem().string(); // "cambios"
      repositoryStore_.cleanWorkspace(identifier);


      // 7. Registrar archivos y permisos en la base de datos
      // aqui si no esta en la tabla "sourcefiles" agregarlo y tambien a "filepermisions" ya que si es nuevo archivo el usuario que hizo el push debe tener permisos automaticamente
      for (const auto& [relativePath, signatureB64] : fileSignatures) {

         // 7.1 Buscar el archivo en la tabla sourcefiles
         auto fileOpt = DBProjectRepository_.existsFileInProject(relativePath, repoOpt->idProject);

         if (!fileOpt.has_value()) {
            // No existe -> lo agregamos al proyecto
            bool fileAdded = DBProjectRepository_.addFileToProject(relativePath, repoOpt->idProject);
            if (!fileAdded) {
               std::cerr << "Failed to add file to project in DB: " << relativePath << std::endl;
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
         int idFile = fileDB.idsourcefile;

         // 7.2 Asignar permisos al usuario que hizo el push, si no los tiene
         int idUser = userOpt->idUser;   // ajuste el nombre del campo si es distinto

         bool hasPerm = DBProjectRepository_.existsUserFilePermission(idUser, idFile);
         if (!hasPerm) {
            bool permAdded = DBProjectRepository_.addUserFilePermission(idUser, idFile);
            if (!permAdded) {
               std::cerr << "Failed to add user-file permission for user " << idUser << " and file " << idFile << std::endl;
               // Puede decidir si continúa o lanza excepción
            }
         }
      }

      return true; // o false, según el resultado real
   }

private:
   IRepositoryStore &repositoryStore_;
   IProjectRepositoryDB &DBProjectRepository_;
   IUserRepository &userRepository_;
   IPushRepoCryptoRepository &cryptoRepo_;
};
