#pragma once
#include <string>
#include <stdexcept>

#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"

class AddUserToRepoUseCase {
public:
   explicit AddUserToRepoUseCase(IProjectRepositoryDB &projectRepositoryDB, IUserRepository &userRepository)
      : projectRepositoryDB_(projectRepositoryDB), userRepository_(userRepository) {}

   bool execute(const std::string &approverEmail, const std::string &approverPassword, const std::string &projectName, const std::string &userEmail) {
      
      // Verificar que el usuario aprobador exista, este activo, verificado y tenga el password correcto
      if (!userRepository_.findByEmail(approverEmail).has_value())
         throw std::runtime_error("Approver user with email " + approverEmail + " does not exist");

      if (!userRepository_.isValidPassword(approverEmail, approverPassword))
         throw std::runtime_error("Invalid password for approver user: " + approverEmail);

      if (!userRepository_.isStatusActive(approverEmail))
         throw std::runtime_error("User: " + approverEmail + " is not active");

      if (!userRepository_.isVerifiedUser(approverEmail))
         throw std::runtime_error("Approver user with email " + approverEmail + " is not verified");


      auto projectOpt = projectRepositoryDB_.findByName(projectName);
      if (!projectOpt.has_value())
         throw std::runtime_error("Project with name " + projectName + " does not exist");


      // Verificar que el usuario a agregar exista
      auto userOpt = userRepository_.findByEmail(userEmail);
      if (!userOpt.has_value())
         throw std::runtime_error("User with email " + userEmail + " does not exist");

      // Verificar que no exista ya la relacion entre el usuario y el proyecto
      if (projectRepositoryDB_.existsUserInProject(projectOpt->idProject, userOpt->idUser))
         throw std::runtime_error("User with email " + userEmail + " is already added to project " + projectName);

      // Verificar que el usuario approver tenga permisos (Senior o Leader del proyecto)
      auto approverUser = userRepository_.findByEmail(approverEmail);

      // Si es senior, permitir a todos los proyectos
      if (userRepository_.isSeniorUser(approverEmail)) {
         return projectRepositoryDB_.addUserToProject(projectOpt->idProject, userOpt->idUser);
      }

      // Si es leader, permitir solo si es el owner del proyecto
      if (userRepository_.isLeaderUser(approverEmail)) {
         if (projectOpt->ownerId == approverUser->idUser) {
            return projectRepositoryDB_.addUserToProject(projectOpt->idProject, userOpt->idUser);
         } else {
            throw std::runtime_error("Leader user with email " + approverEmail + " is not the owner of the project " + projectName);
         }
      }

      throw std::runtime_error("User " + approverEmail + " is not authorized to add users to the project " + projectName);
   }

private:
   IProjectRepositoryDB &projectRepositoryDB_;
   IUserRepository      &userRepository_;
};