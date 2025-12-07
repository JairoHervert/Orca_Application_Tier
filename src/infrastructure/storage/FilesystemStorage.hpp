// infrastructure/storage/FilesystemStorage.hpp
#pragma once
#include "../../domain/repositories/IRepositoryStore.repository.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <optional>
#include <stdexcept>

class FilesystemStorage : public IRepositoryStore {
public:
   explicit FilesystemStorage(const std::filesystem::path& repositoriesRoot, const std::filesystem::path& cipherPath, const std::filesystem::path& workspacePath)
      : rootPath_(repositoriesRoot), cipherPath_(cipherPath), workspacePath_(workspacePath) {
      // Si la carpeta raíz no existe, crearla
      if (!std::filesystem::exists(rootPath_)) {
         std::filesystem::create_directories(rootPath_);
      }

      // Si la carpeta de cifrado no existe, crearla
      if (!std::filesystem::exists(cipherPath_)) {
         std::filesystem::create_directories(cipherPath_);
      }

      // Si la carpeta de workspace no existe, crearla
      if (!std::filesystem::exists(workspacePath_)) {
         std::filesystem::create_directories(workspacePath_);
      }
   }

   std::optional<Repository> findByName(const std::string &name) override {
      // ruta: <root>/name
      std::filesystem::path repoPath = rootPath_ / name;

      if (std::filesystem::exists(repoPath) &&
          std::filesystem::is_directory(repoPath)) {

         Repository repo;
         repo.name = name;
         return repo;
      }

      return std::nullopt;
   }

   std::optional<Repository> findByNameInCiphers(const std::string &name) override {
      // ruta: <cipherPath>/name.enc
      std::filesystem::path cipherFilePath = cipherPath_ / (name + ".tar.enc");

      if (std::filesystem::exists(cipherFilePath) &&
          std::filesystem::is_regular_file(cipherFilePath)) {

         Repository repo;
         repo.name = name;
         return repo;
      }

      return std::nullopt;
   }

   Repository create(const std::string &name) override {
      std::filesystem::path repoPath = rootPath_ / name;

      if (std::filesystem::exists(repoPath)) {
         throw std::runtime_error("Repository directory already exists on disk");
      }

      // Crear carpeta del repositorio
      std::filesystem::create_directories(repoPath);

      // Devolver entidad de dominio
      Repository repo;
      repo.name = name;
      return repo;
   }

   
   bool deleteRepositoryFolder(const std::string &name) {
      std::filesystem::path repoPath = rootPath_ / name;

      if (!std::filesystem::exists(repoPath)) return false; // No existe
      if (!std::filesystem::is_directory(repoPath)) return false; // No es carpeta

      // Eliminar carpeta del repositorio y su contenido
      try {
         std::filesystem::remove_all(repoPath);
      } catch (const std::exception &e) {
         throw std::runtime_error("Error deleting repository folder: " + std::string(e.what()));
      }

      return true;
   }

   bool deleteRepositoryFile(const std::string &name) {
      std::filesystem::path repoFilePath = rootPath_ / name;

      if (!std::filesystem::exists(repoFilePath)) return false; // No existe
      if (!std::filesystem::is_regular_file(repoFilePath)) return false; // No es archivo

      // Eliminar archivo del repositorio
      try {
         std::filesystem::remove(repoFilePath);
      } catch (const std::exception &e) {
         throw std::runtime_error("Error deleting repository file: " + std::string(e.what()));
      }

      return true;
   }

   bool deleteCipherFile(const std::string &name) {
      std::filesystem::path cipherFilePath = cipherPath_ / name;

      if (!std::filesystem::exists(cipherFilePath)) return false; // No existe
      if (!std::filesystem::is_regular_file(cipherFilePath)) return false; // No es archivo

      // Eliminar archivo cifrado del repositorio
      try {
         std::filesystem::remove(cipherFilePath);
      } catch (const std::exception &e) {
         throw std::runtime_error("Error deleting cipher file: " + std::string(e.what()));
      }

      return true;
   }


   // Funcion para convertir una carpeta en un archivo .tar
   std::filesystem::path folderToTar(const std::string &name, const std::string &projectAlias) override {
      std::filesystem::path repoPath = rootPath_ / name;
      std::filesystem::path tarPath = cipherPath_ / (name + "_" + projectAlias + ".tar");
      
      // Validar que el repositorio exista
      if (!std::filesystem::exists(repoPath))
         throw std::runtime_error("Repository directory does not exist: " + name);
      
      if (!std::filesystem::is_directory(repoPath))
         throw std::runtime_error("Path is not a directory: " + name);
      
      // Crear el comando tar
      // -c: crear archivo, -f: archivo de salida, -C: cambiar a directorio
      std::string command = "tar -czf \"" + tarPath.string() + "\" -C \"" + rootPath_.string() + "\" \"" + name + "\"";
      
      int result = std::system(command.c_str());
      
      if (result != 0)
         throw std::runtime_error("Failed to create tar archive for: " + name);
      
      // Verificar que el archivo tar se creó correctamente
      if (!std::filesystem::exists(tarPath))
         throw std::runtime_error("Tar file was not created: " + tarPath.string());
      
      return tarPath;
   }

   
   // Funcion para extraer un archivo .tar (o .tar.gz) a una carpeta
   std::filesystem::path tarToFolder(const std::filesystem::path &tarPath) override {
      if (!std::filesystem::exists(tarPath))
         throw std::runtime_error("Tar file does not exist: " + tarPath.string());

      if (!std::filesystem::is_regular_file(tarPath))
         throw std::runtime_error("Path is not a regular file: " + tarPath.string());

      // Nombre base de la carpeta destino, a partir del nombre del archivo
      // Ej: cripto22.tar        -> cripto22
      //     cripto22_dec.tar    -> cripto22_dec
      std::string repoName = tarPath.stem().string();
      std::filesystem::path repoPath = rootPath_ / repoName;

      if (std::filesystem::exists(repoPath))
         throw std::runtime_error("Repository directory already exists: " + repoPath.string());

      // Crear el directorio destino
      std::filesystem::create_directories(repoPath);

      // Extraer el tar en esa carpeta
      // OJO: tu tar se creó con: tar -czf "<tarPath>" -C "<rootPath_>" "<name>"
      // Eso significa que dentro del tar hay una carpeta "<name>/..."
      //
      // Para que el contenido quede directo en repoPath (sin carpeta extra),
      // usamos --strip-components=1
      std::string command =
         "tar -xzf \"" + tarPath.string() + "\" "
         "-C \"" + repoPath.string() + "\" "
         "--strip-components=1";

      int result = std::system(command.c_str());
      if (result != 0)
         throw std::runtime_error("Failed to extract tar archive: " + tarPath.string());

      // Verificar que algo se haya creado (opcional)
      if (!std::filesystem::exists(repoPath))
         throw std::runtime_error("Repository directory was not created: " + repoPath.string());

      return repoPath;
   }

   std::ostringstream getFileAsStream(const std::string &filePath, const std::string &name) {
      std::string ruta = filePath + "/" + name;

      std::ifstream fileStream(ruta, std::ios::binary);

      std::ostringstream oss;
      oss << fileStream.rdbuf();
      return oss;
   }

   // Obtener el ostringstream un repositorio y de la carpeta de los cifrados usando la funcion anterior
   std::ostringstream getRepoAsStream(const std::string &name) {
      return getFileAsStream(rootPath_.string(), name);
   }

   std::ostringstream getCipherAsStream(const std::string &name) {
      return getFileAsStream(cipherPath_.string(), name);
   }

   std::vector<std::filesystem::path> listAllFiles(const std::string &repoName) override {
      std::filesystem::path repoPath = rootPath_ / repoName;
      
      if (!std::filesystem::exists(repoPath))
         throw std::runtime_error("Repository does not exist: " + repoName);
      
      if (!std::filesystem::is_directory(repoPath))
         throw std::runtime_error("Path is not a directory: " + repoName);
      
      std::vector<std::filesystem::path> files;
      
      // Recorrer recursivamente todos los archivos
      for (const auto& entry : std::filesystem::recursive_directory_iterator(repoPath)) {
         if (entry.is_regular_file()) {
            // Guardar la ruta relativa al repositorio
            std::filesystem::path relativePath = std::filesystem::relative(entry.path(), repoPath);
            files.push_back(relativePath);
         }
      }
      
      return files;
   }

   std::string getFullPath(const std::string &repoName, const std::string &relativePath) override {
      std::filesystem::path fullPath = rootPath_ / repoName / relativePath;
      return fullPath.string();
   }

   // === IMPLEMENTACIÓN DE MÉTODOS PARA WORKSPACE ===

   // Guardar archivo .tar recibido en workspace
   std::filesystem::path saveTarToWorkspace(const std::string &tarContent, const std::string &filename) override {
      std::filesystem::path tarPath = workspacePath_ / filename;
      
      try {
         // Crear directorio workspace si no existe
         if (!std::filesystem::exists(workspacePath_)) {
               std::filesystem::create_directories(workspacePath_);
         }
         
         // Guardar el contenido del tar
         std::ofstream outFile(tarPath, std::ios::binary | std::ios::trunc);
         if (!outFile.is_open()) {
               throw std::runtime_error("Could not open file for writing: " + tarPath.string());
         }
         
         outFile.write(tarContent.c_str(), tarContent.length());
         outFile.close();
         
         if (!outFile.good()) {
               throw std::runtime_error("Error writing tar file: " + tarPath.string());
         }
         
         std::cout << "✓ Tar saved to workspace: " << tarPath.string() 
                     << " (" << tarContent.length() << " bytes)" << std::endl;
         
         return tarPath;
         
      } catch (const std::exception &e) {
         throw std::runtime_error("Error saving tar to workspace: " + std::string(e.what()));
      }
   }

   // Extraer .tar en workspace
   std::filesystem::path extractTarInWorkspace(const std::string &tarFilename) override {
      std::filesystem::path tarPath = workspacePath_ / tarFilename;
      
      if (!std::filesystem::exists(tarPath)) {
         throw std::runtime_error("Tar file does not exist in workspace: " + tarPath.string());
      }
      
      if (!std::filesystem::is_regular_file(tarPath)) {
         throw std::runtime_error("Path is not a regular file: " + tarPath.string());
      }
      
      // Crear carpeta de extracción con el nombre del tar (sin extensión)
      std::string extractDirName = tarPath.stem().string() + "_extracted";
      std::filesystem::path extractPath = workspacePath_ / extractDirName;
      
      if (std::filesystem::exists(extractPath)) {
         // Si ya existe, eliminarla primero
         std::filesystem::remove_all(extractPath);
      }
      
      // Crear directorio de extracción
      std::filesystem::create_directories(extractPath);
      
      // Extraer el tar
      std::string command = "tar -xzf \"" + tarPath.string() + "\" -C \"" + extractPath.string() + "\"";
      
      int result = std::system(command.c_str());
      if (result != 0) {
         throw std::runtime_error("Failed to extract tar archive: " + tarPath.string());
      }
      
      // Verificar que se extrajo algo
      if (!std::filesystem::exists(extractPath) || std::filesystem::is_empty(extractPath)) {
         throw std::runtime_error("Extraction failed or resulted in empty directory: " + extractPath.string());
      }
      
      std::cout << "✓ Tar extracted to: " << extractPath.string() << std::endl;
      
      return extractPath;
   }

   // Limpiar archivos temporales del workspace
   bool cleanWorkspace(const std::string &identifier) override {
      try {
         bool cleaned = false;
         
         // Buscar y eliminar archivos/carpetas que contengan el identificador
         for (const auto& entry : std::filesystem::directory_iterator(workspacePath_)) {
               std::string filename = entry.path().filename().string();
               
               // Si el nombre contiene el identificador, eliminarlo
               if (filename.find(identifier) != std::string::npos) {
                  if (std::filesystem::is_directory(entry.path())) {
                     std::filesystem::remove_all(entry.path());
                     std::cout << "✓ Removed workspace directory: " << entry.path() << std::endl;
                  } else {
                     std::filesystem::remove(entry.path());
                     std::cout << "✓ Removed workspace file: " << entry.path() << std::endl;
                  }
                  cleaned = true;
               }
         }
         
         return cleaned;
         
      } catch (const std::exception &e) {
         std::cerr << "Error cleaning workspace: " << e.what() << std::endl;
         return false;
      }
   }

   // Obtener ruta completa en workspace
   std::string getWorkspacePath(const std::string &relativePath) override {
      std::filesystem::path fullPath = workspacePath_ / relativePath;
      return fullPath.string();
   }



   void updateFileFromWorkspace(const std::string &repoName, const std::filesystem::path &extractedBase, const std::filesystem::path &relativePath) override {
      try {
         // src: carpeta donde se extrajo el tar
         std::filesystem::path srcPath = extractedBase / relativePath;

         if (!std::filesystem::exists(srcPath) || !std::filesystem::is_regular_file(srcPath)) {
            throw std::runtime_error("Source file does not exist in workspace: " + srcPath.string());
         }

         // dst: ruta final del archivo en el repo real
         std::filesystem::path dstPath =
            std::filesystem::path(getFullPath(repoName, relativePath.string()));

         // crear directorios padre si no existen
         std::filesystem::create_directories(dstPath.parent_path());

         // copiar el archivo, sobreescribiendo
         std::filesystem::copy_file(
            srcPath,
            dstPath,
            std::filesystem::copy_options::overwrite_existing
         );

         std::cout << "✓ Updated repo file: " << dstPath.string() << std::endl;

      } catch (const std::exception &e) {
         throw std::runtime_error(
            std::string("Error updating file from workspace: ") + e.what()
         );
      }
   }

   bool deleteFile(const std::string &repoName, const std::filesystem::path &relativePath) override {
      try {
         // Ruta completa: <rootPath_>/<repoName>/<relativePath>
         std::filesystem::path fullPath = rootPath_ / repoName / relativePath;

         if (!std::filesystem::exists(fullPath)) {
            std::cerr << "[FilesystemStorage::deleteFile] File does not exist: " << fullPath << std::endl;
            return false;
         }

         if (!std::filesystem::is_regular_file(fullPath)) {
            std::cerr << "[FilesystemStorage::deleteFile] Not a regular file: " << fullPath << std::endl;
            return false;
         }

         std::filesystem::remove(fullPath);

         std::cout << "✓ Deleted repo file: " << fullPath.string() << std::endl;
         return true;

      } catch (const std::exception &e) {
         std::cerr << "[FilesystemStorage::deleteFile] " << e.what() << std::endl;
         return false;
      }
   }

private:
   std::filesystem::path rootPath_;
   std::filesystem::path cipherPath_;
   std::filesystem::path workspacePath_;
};
