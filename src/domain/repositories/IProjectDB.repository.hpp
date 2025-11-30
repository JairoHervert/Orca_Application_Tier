#pragma once
#include <optional>
#include <string>
#include "../entities/Repository.entity.hpp"

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

};
