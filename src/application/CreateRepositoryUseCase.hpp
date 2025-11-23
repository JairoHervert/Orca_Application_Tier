#pragma once
#include <string>
#include <stdexcept>
// Caso de uso para crear un repositorio
#include "../domain/entities/Repository.entity.hpp"
#include "../domain/repositories/IRepositoryStore.repository.hpp"

// Caso de uso para usuarios en ls base de datos
#include "../domain/entities/User.entity.hpp"
#include "../domain/repositories/IUserRepository.user.hpp"

class CreateRepositoryUseCase {
public:
   explicit CreateRepositoryUseCase(
      IRepositoryStore &repositoryStore,     // inyección de dependencia FilesystemStorage
      IUserRepository &userRepository)       // inyección de dependencia DBUserRepository
      : repositoryStore_(repositoryStore),
        userRepository_(userRepository) {}

   Repository execute(const std::string &repoName, const std::string &userEmail) {

      // 1. Validar que el usuario que desea crear el repo exista y sea tipo Lider
      auto ownerOpt = userRepository_.findByEmail(userEmail);
      if (!ownerOpt.has_value()) 
         throw std::runtime_error("User: " + userEmail + " not found");

      if (!userRepository_.isLeaderUser(userEmail))
         throw std::runtime_error(userEmail + " is not authorized to create a repository");
      
      // Regla de negocio: no permitir duplicados
      auto existing = repositoryStore_.findByName(repoName);
      if (existing.has_value()) {
         throw std::runtime_error("Repository" + repoName + "already exists");
      }

      return repositoryStore_.create(repoName, ownerOpt->email);
   }

private:
   IRepositoryStore &repositoryStore_;
   IUserRepository  &userRepository_;
};
