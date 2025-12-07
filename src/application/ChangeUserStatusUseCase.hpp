#pragma once
#include <string>
#include <stdexcept>

// repositorios de operaciones con usuarios en la base de datos
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"

class ChangeStatusUserUseCase {
public:
   explicit ChangeStatusUserUseCase(
      IUserRepository &userRepository,
      IProjectRepositoryDB &projectRepositoryDB)
      : userRepository_(userRepository),
        projectRepositoryDB_(projectRepositoryDB) {}

   bool execute(const std::string &approverEmail,
                const std::string &approverPassword,
                const std::string &targetUserEmail,
                int newStatus) {

      try {
         // Verificar que el nuevo status sea valido (0: no trabaja actualmente, 1: trabaja actualmente)
         if (newStatus < 0 || newStatus > 1)
            throw std::runtime_error(
               "Invalid status value: " + std::to_string(newStatus) +
               ". Must be 0 (Inactive - not working) or 1 (Active - working)");

         // Verificar que el usuario aprobador exista
         auto approverOpt = userRepository_.findByEmail(approverEmail);
         if (!approverOpt.has_value())
            throw std::runtime_error("Approver user with email " + approverEmail + " does not exist");

         // Verificar que el password del aprobador sea correcto
         if (!userRepository_.isValidPassword(approverEmail, approverPassword))
            throw std::runtime_error("Invalid password for approver user: " + approverEmail);

         // verificar que el status del usuario aprobador sea activo (esta trabajando actualmente)
         if (!userRepository_.isStatusActive(approverEmail))
            throw std::runtime_error("User: " + approverEmail + " is not active");

         // Verificar que el usuario aprobador este verificado
         if (!userRepository_.isVerifiedUser(approverEmail))
            throw std::runtime_error("Approver user with email " + approverEmail + " is not verified");

         // Verificar que el usuario aprobador tenga permisos (Senior)
         if (!userRepository_.isSeniorUser(approverEmail))
            throw std::runtime_error("User " + approverEmail + " is not authorized to change user status"); 

         // Verificar que el usuario objetivo exista
         auto targetOpt = userRepository_.findByEmail(targetUserEmail);
         if (!targetOpt.has_value())
            throw std::runtime_error("Target user with email " + targetUserEmail + " does not exist");

         // Cambiar el status del usuario objetivo
         bool ok = userRepository_.changeActiveStatus(targetUserEmail, newStatus);

         // Registrar commit de éxito
         projectRepositoryDB_.addCommit(
            approverOpt->idUser,
            std::nullopt,                     // sin archivo asociado
            std::nullopt,                     // sin firma
            true,                             // isAccepted
            "CHANGE_STATUS_USER",             // command
            "Changed status of user " + targetUserEmail +
               " to " + std::to_string(newStatus) +
               " by approver " + approverEmail
         );

         return ok;

      } catch (const std::exception &e) {
         // Intentar recuperar id del aprobador si existe, para log
         int approverId = -1;
         auto approverOpt = userRepository_.findByEmail(approverEmail);
         if (approverOpt.has_value()) {
            approverId = approverOpt->idUser;
         }

         // Registrar commit de fallo
         projectRepositoryDB_.addCommit(
            approverId,
            std::nullopt,
            std::nullopt,
            false,
            "CHANGE_STATUS_USER",
            std::string("Failed to change status for user ") +
               targetUserEmail + ": " + e.what()
         );

         throw; // propagar el error al llamador
      }
   }

private:
   IUserRepository  &userRepository_;
   IProjectRepositoryDB &projectRepositoryDB_;
};
