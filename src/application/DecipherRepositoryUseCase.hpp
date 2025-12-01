#pragma once
#include <string>
#include <sstream>
#include <stdexcept>
#include "../domain/repositories/IRepositoryStore.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"

class DecipherRepositoryUseCase {
public:
   explicit DecipherRepositoryUseCase(
      IRepositoryStore &repositoryStore,
      IUserRepository &userRepository,
      IProjectRepositoryDB &projectRepositoryDB)
      : repositoryStore_(repositoryStore),
         userRepository_(userRepository),
         projectRepositoryDB_(projectRepositoryDB) {}

   std::ostringstream execute(const std::string &repoName, const std::string &userEmail, const std::string &userPassword) {
      
      // Verificar existencia de los actores
      auto userOpt = userRepository_.findByEmail(userEmail);
      
      if (!userOpt.has_value())
         throw std::runtime_error("User with email " + userEmail + " does not exist");

      if (!projectRepositoryDB_.existsRepoAlias(repoName))
         throw std::runtime_error("Project with name " + repoName + " does not exist in database");

      if (!repositoryStore_.findByNameInCiphers(repoName).has_value())
         throw std::runtime_error("Encrypted repository not found: " + repoName);


      if (!userRepository_.isValidPassword(userEmail, userPassword))
         throw std::runtime_error("Invalid password for user with email " + userEmail);
      

      // Verificar que el usuario este activo y verificado
      if (!userRepository_.isStatusActive(userEmail))
         throw std::runtime_error("User with email " + userEmail + " is not active");
      if (!userRepository_.isVerifiedUser(userEmail))
         throw std::runtime_error("User with email " + userEmail + " is not verified");

      // Solo el senior o usuarios asociados al proyecto pueden descifrarlo
      if (projectRepositoryDB_.existsUserInCipherProject(repoName, userOpt->idUser)) {
         return repositoryStore_.getCipherAsStream(repoName + ".tar.enc");
      } else {
         throw std::runtime_error("User with email " + userEmail + " does not have access to decipher project " + repoName);
      }
   }

private:
   IRepositoryStore &repositoryStore_;
   IUserRepository &userRepository_;
   IProjectRepositoryDB &projectRepositoryDB_;
};