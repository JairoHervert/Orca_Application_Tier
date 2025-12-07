#pragma once
#include <optional>
#include <string>
#include <filesystem>
#include "../entities/Repository.entity.hpp"

class IRepositoryStore {
public:
   virtual ~IRepositoryStore() = default;

   virtual std::optional<Repository> findByName(const std::string &name) = 0;

   virtual std::optional<Repository> findByNameInCiphers(const std::string &name) = 0;

   virtual Repository create(const std::string &name) = 0;

   virtual bool deleteRepositoryFolder(const std::string &name) = 0;

   virtual bool deleteRepositoryFile(const std::string &name) = 0;

   virtual bool deleteCipherFile(const std::string &name) = 0;

   virtual std::filesystem::path folderToTar(const std::string &name, const std::string &projectAlias) = 0;

   virtual std::filesystem::path tarToFolder(const std::filesystem::path &tarPath) = 0;

   virtual std::ostringstream getFileAsStream(const std::string &filePath, const std::string &name) = 0;

   virtual std::ostringstream getRepoAsStream(const std::string &name) = 0;

   virtual std::ostringstream getCipherAsStream(const std::string &name) = 0;

   virtual std::vector<std::filesystem::path> listAllFiles(const std::string &repoName) = 0;

   virtual std::string getFullPath(const std::string &repoName, const std::string &relativePath) = 0;

   // === NUEVOS MÉTODOS PARA WORKSPACE ===
   
   // Guardar archivo .tar en workspace
   virtual std::filesystem::path saveTarToWorkspace(const std::string &tarContent, const std::string &filename) = 0;
   
   // Extraer .tar en workspace
   virtual std::filesystem::path extractTarInWorkspace(const std::string &tarFilename) = 0;
   
   // Limpiar archivos temporales del workspace
   virtual bool cleanWorkspace(const std::string &identifier) = 0;
   
   // Obtener ruta completa de un archivo en workspace
   virtual std::string getWorkspacePath(const std::string &relativePath) = 0;

   // Copiar un único archivo desde workspace al repo real
   virtual void updateFileFromWorkspace(const std::string &repoName, const std::filesystem::path &extractedBase, const std::filesystem::path &relativePath) = 0;


   // IRepositoryStore.repository.hpp
   virtual bool deleteFile(const std::string &repoName, const std::filesystem::path &relativePath) = 0;

};
