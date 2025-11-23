#pragma once
#include <string>
#include <stdexcept>
// Caso de uso para crear un repositorio
#include "../domain/entities/Repository.entity.hpp"
#include "../domain/repositories/IRepositoryStore.repository.hpp"

// Caso de uso para usuarios en ls base de datos
#include "../domain/entities/User.entity.hpp"
#include "../domain/repositories/IUser.repository.hpp"

class CreateRepositoryUseCase {
public:
   explicit CreateRepositoryUseCase(
      IRepositoryStore &repositoryStore,     // inyección de dependencia FilesystemStorage
      IUserRepository &userRepository)       // inyección de dependencia DBUserRepository
      : repositoryStore_(repositoryStore),
        userRepository_(userRepository) {}

   Repository execute(const std::string &repoName, const std::string &userEmail, const std::string &userPassword) {

      // 1.0. Validar que el usuario que desea crear el repo exista
      auto ownerOpt = userRepository_.findByEmail(userEmail);
      if (!ownerOpt.has_value()) 
         throw std::runtime_error("User: " + userEmail + " not found");

      // 1.1 Validar que el password sea correcto
      if (!userRepository_.isValidPassword(userEmail, userPassword))
         throw std::runtime_error("Invalid password for user: " + userEmail);

      // 1.2. Validar que el usuario tenga permisos para crear repositorios
      if (!userRepository_.isLeaderUser(userEmail))
         throw std::runtime_error(userEmail + " is not authorized to create a repository");
 
      // Regla de negocio: no permitir duplicados
      auto existing = repositoryStore_.findByName(repoName);
      if (existing.has_value()) {
         throw std::runtime_error("Repository" + repoName + " already exists");
      }

      return repositoryStore_.create(repoName, userEmail);
   }

private:
   IRepositoryStore &repositoryStore_;
   IUserRepository  &userRepository_;
};
