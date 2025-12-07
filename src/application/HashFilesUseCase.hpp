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

   std::map<std::string, std::string> execute(const std::string &repoName, const std::string &userEmail, const std::string &userPassword) {
      // 1-3. Validaciones...
      
      // 4. Listar todos los archivos del repositorio
      std::vector<std::filesystem::path> files = repositoryStore_.listAllFiles(repoName);

      // 5. Calcular hash de cada archivo
      std::map<std::string, std::string> fileHashes;
      
      for (const auto& relativePath : files) {
         // Obtener ruta completa para calcular hash
         std::string fullPathStr = repositoryStore_.getFullPath(repoName, relativePath.string());
         
         // Calcular hash
         std::string hashB64 = cryptoRepo_.b64_hash_file_SHA256(fullPathStr);
         
         if (hashB64.empty()) {
            std::cerr << "Warning: Failed to hash file: " << fullPathStr << std::endl;
            continue;
         }
         
         // Usar directamente la ruta relativa (a partir de la raíz del repo) : carpeta/archivo.txt
         fileHashes[relativePath.string()] = hashB64;
      }

      std::cout << "Hashed " << fileHashes.size() << " files from repository " << repoName << std::endl;
      
      return fileHashes;
   }

private:
   IRepositoryStore &repositoryStore_;
   IProjectRepositoryDB &DBProjectRepository_;
   IUserRepository &userRepository_;
   IPushRepoCryptoRepository &cryptoRepo_;
};