// infrastructure/storage/FilesystemStorage.hpp
#pragma once
#include "../../domain/repositories/IRepositoryStore.repository.hpp"
#include <filesystem>
#include <optional>
#include <stdexcept>

class FilesystemStorage : public IRepositoryStore {
public:
   explicit FilesystemStorage(const std::filesystem::path& repositoriesRoot)
      : rootPath_(repositoriesRoot) {
      // Si la carpeta raíz no existe, crearla
      if (!std::filesystem::exists(rootPath_)) {
         std::filesystem::create_directories(rootPath_);
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

private:
   std::filesystem::path rootPath_;
};
