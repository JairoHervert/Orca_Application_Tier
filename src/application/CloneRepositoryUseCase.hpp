#pragma once
#include <string>
#include <sstream>
#include <stdexcept>
#include "../domain/repositories/IRepositoryStore.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"

class CloneRepositoryUseCase {
public:
   explicit CloneRepositoryUseCase(
      IRepositoryStore &repositoryStore,
      IUserRepository &userRepository,
      IProjectRepositoryDB &projectRepositoryDB)
      : repositoryStore_(repositoryStore),
         userRepository_(userRepository),
         projectRepositoryDB_(projectRepositoryDB) {}

   std::ostringstream execute(const std::string &repoName, const std::string &userEmail, const std::string &userPassword) {

      // Verificar existencia de los actores
      auto userOpt = userRepository_.findByEmail(userEmail);
      auto projectOpt = projectRepositoryDB_.findByName(repoName);
      
      if (!userOpt.has_value())
         throw std::runtime_error("User with email " + userEmail + " does not exist");

      if (!projectOpt.has_value())
         throw std::runtime_error("Project with name " + repoName + " does not exist in database");

      if (!repositoryStore_.findByName(repoName).has_value())
         throw std::runtime_error("Repository with name " + repoName + " does not exist in filesystem");

      
      // Verificar que el password sea correcto
      if (!userRepository_.isValidPassword(userEmail, userPassword))
         throw std::runtime_error("Invalid password for user with email " + userEmail);

      // Verificar que el usuario este activo y verificado
      if (!userRepository_.isStatusActive(userEmail))
         throw std::runtime_error("User with email " + userEmail + " is not active");
      if (!userRepository_.isVerifiedUser(userEmail))
         throw std::runtime_error("User with email " + userEmail + " is not verified");

      // Verificar algun tipo de acceso al proyecto
      // ser senior, ser owner del proyecto, o estar en la lista de usuarios del proyecto
      bool accessGranted = false;
      if (userRepository_.isSeniorUser(userEmail)) {
         accessGranted = true;
      } else if (projectOpt->ownerId == userOpt->idUser) {
         accessGranted = true;
      } else if (projectRepositoryDB_.existsUserInProject(projectOpt->idProject, userOpt->idUser)) {
         accessGranted = true;
      }

      if (!accessGranted)
         throw std::runtime_error("User with email " + userEmail + " does not have access to project " + repoName);

      // Crear un .tar del repositorio
      std::filesystem::path tarPath = repositoryStore_.folderToTar(repoName, "clone_temp");

      // Obtener el .tar como stream
      std::ostringstream repoStream = repositoryStore_.getFileAsStream(tarPath.parent_path().string(), tarPath.filename().string());

      // Eliminar el .tar temporal
      std::filesystem::remove(tarPath);

      return repoStream;
   }

private:
   IRepositoryStore &repositoryStore_;
   IUserRepository &userRepository_;
   IProjectRepositoryDB &projectRepositoryDB_;
};