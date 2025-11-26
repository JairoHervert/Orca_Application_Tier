// infrastructure/storage/FilesystemStorage.hpp
#pragma once
#include "../../domain/repositories/IRepositoryStore.repository.hpp"
#include <filesystem>
#include <optional>
#include <stdexcept>

class FilesystemStorage : public IRepositoryStore {
public:
   explicit FilesystemStorage(const std::filesystem::path& repositoriesRoot, const std::filesystem::path& cipherPath)
      : rootPath_(repositoriesRoot), cipherPath_(cipherPath) {
      // Si la carpeta raíz no existe, crearla
      if (!std::filesystem::exists(rootPath_)) {
         std::filesystem::create_directories(rootPath_);
      }

      // Si la carpeta de cifrado no existe, crearla
      if (!std::filesystem::exists(cipherPath_)) {
         std::filesystem::create_directories(cipherPath_);
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


private:
   std::filesystem::path rootPath_;
   std::filesystem::path cipherPath_;
};
