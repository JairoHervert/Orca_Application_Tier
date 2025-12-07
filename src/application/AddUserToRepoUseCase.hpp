#pragma once
#include <string>
#include <stdexcept>
#include <optional>   // <-- para std::optional y std::nullopt

#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"

class AddUserToRepoUseCase {
public:
   explicit AddUserToRepoUseCase(IProjectRepositoryDB &projectRepositoryDB,
                                 IUserRepository &userRepository)
      : projectRepositoryDB_(projectRepositoryDB),
        userRepository_(userRepository) {}

   bool execute(const std::string &approverEmail,
                const std::string &approverPassword,
                const std::string &projectName,
                const std::string &userEmail) {

      int approverId = -1;   // para registrar en commits quién hizo la acción
      int projectId  = -1;   // opcional, por si después quieres usarlo en la descripción

      try {
         // Verificar que el usuario aprobador exista
         auto approverOpt = userRepository_.findByEmail(approverEmail);
         if (!approverOpt.has_value())
            throw std::runtime_error("Approver user with email " + approverEmail + " does not exist");

         approverId = approverOpt->idUser;

         // Verificar que el password del aprobador sea correcto
         if (!userRepository_.isValidPassword(approverEmail, approverPassword))
            throw std::runtime_error("Invalid password for approver user: " + approverEmail);

         // verificar que el status del usuario aprobador sea activo (esta trabajando actualmente)
         if (!userRepository_.isStatusActive(approverEmail))
            throw std::runtime_error("User: " + approverEmail + " is not active");

         // Verificar que el usuario aprobador este verificado
         if (!userRepository_.isVerifiedUser(approverEmail))
            throw std::runtime_error("Approver user with email " + approverEmail + " is not verified");

         // Verificar que el proyecto exista
         auto projectOpt = projectRepositoryDB_.findByName(projectName);
         if (!projectOpt.has_value())
            throw std::runtime_error("Project with name " + projectName + " does not exist");

         projectId = projectOpt->idProject;

         // Verificar que el usuario a agregar exista
         auto userOpt = userRepository_.findByEmail(userEmail);
         if (!userOpt.has_value())
            throw std::runtime_error("User with email " + userEmail + " does not exist");

         // Verificar que el usuario a agregar sea activo y verificado
         if (!userRepository_.isStatusActive(userEmail))
            throw std::runtime_error("User with email " + userEmail + " is not active");
         if (!userRepository_.isVerifiedUser(userEmail))
            throw std::runtime_error("User with email " + userEmail + " is not verified");

         // Verificar que no exista ya la relacion entre el usuario y el proyecto
         if (projectRepositoryDB_.existsUserInProject(projectOpt->idProject, userOpt->idUser))
            throw std::runtime_error("User with email " + userEmail + " is already added to project " + projectName);

         bool added = false;

         // Si es senior, permitir a todos los proyectos
         if (userRepository_.isSeniorUser(approverEmail)) {
            added = projectRepositoryDB_.addUserToProject(projectOpt->idProject, userOpt->idUser);
         }
         // Si es leader, permitir solo si es el owner del proyecto
         else if (userRepository_.isLeaderUser(approverEmail)) {
            if (projectOpt->ownerId == approverOpt->idUser) {
               added = projectRepositoryDB_.addUserToProject(projectOpt->idProject, userOpt->idUser);
            } else {
               throw std::runtime_error(
                  "Leader user with email " + approverEmail +
                  " is not the owner of the project " + projectName
               );
            }
         } else {
            throw std::runtime_error(
               "User " + approverEmail +
               " is not authorized to add users to the project " + projectName
            );
         }

         // Registrar commit de éxito
         projectRepositoryDB_.addCommit(
            approverId,                 // idUser que realiza la operación
            std::nullopt,               // idFile (no aplica)
            std::nullopt,               // signature (no aplica)
            true,                       // isAccepted
            "ADD_USER_TO_PROJECT",      // command
            "User " + userEmail +
               " was added to project " + projectName +
               " by approver " + approverEmail
         );

         return added;

      } catch (const std::exception &e) {
         // Registrar commit de fallo (si no se tenía approverId, se manda -1)
         projectRepositoryDB_.addCommit(
            approverId,                 // puede ser -1 si falló antes de obtenerlo
            std::nullopt,
            std::nullopt,
            false,                      // isAccepted = false
            "ADD_USER_TO_PROJECT",
            std::string("Failed to add user ") + userEmail +
               " to project " + projectName +
               " by approver " + approverEmail + ": " + e.what()
         );
         throw; // propagar el error al controlador HTTP o quien llame al caso de uso
      }
   }

private:
   IProjectRepositoryDB &projectRepositoryDB_;
   IUserRepository      &userRepository_;
};
