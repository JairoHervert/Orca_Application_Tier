#pragma once
#include <optional>
#include <string>
#include <vector>
#include "../entities/Repository.entity.hpp"
#include "../entities/SourceFileDB.entity.hpp"
#include "../entities/Commit.entity.hpp"

class IProjectRepositoryDB {
public:
   virtual ~IProjectRepositoryDB() = default;

   virtual std::optional<Repository> findById(int idProject) = 0;

   virtual std::optional<Repository> findByName(const std::string &name) = 0;

   virtual Repository create(const std::string &name, const std::string &description, int ownerId) = 0;

   virtual bool deleteRepositoryById(int idProject) = 0;

   virtual bool deleteRepositoryByName(const std::string &name) = 0;

   virtual bool existsUserInProject(int idProject, int idUser) = 0;

   virtual bool addUserToProject(int idProject, int idUser) = 0;

   /************* Tabla de passwords/usuarios para repositorios cifrados *************/
   virtual bool addPassword_repo_user(int idUser, int idproject, std::string password, std::string projectAlias) = 0;

   virtual bool existsRepoAlias(const std::string &projectAlias) = 0;

   virtual bool existsUserInCipherProject(const std::string &projectAlias, int idUser) = 0;


   /************* Tabla sourcefiles y filepermissions *************/
   virtual std::optional<SourceFileDB> existsFileInProject(const std::string &relativePath, int idProject) = 0;
   virtual bool addFileToProject(const std::string &relativePath, int idProject) = 0;

   virtual bool existsUserFilePermission(int idUser, int idFile) = 0;
   virtual bool addUserFilePermission(int idUser, int idFile) = 0;


   /************* acciones sobre la tabla de commits *************/
   virtual bool addCommit(
      int idUser,
      std::optional<int> idFile,
      std::optional<std::string> signature,
      bool isAccepted,
      const std::string &command,
      const std::string &description) = 0;


   virtual std::vector<Commit> getCommits() = 0;


   virtual bool deleteFileFromProject(const std::string &relativePath, int idProject) = 0;

   virtual std::string getAESKeyForRepo(int iduser, int idproject, const std::string &project_alias) = 0;

   virtual std::vector<Repository> getAllProjects() = 0;

   virtual std::vector<Repository> getEncryptedProjects() = 0;
};
