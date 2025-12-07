#pragma once
#include <string>
#include <stdexcept>

// repositorios de operaciones con usuarios en la base de datos
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"

class VerifyUserUseCase {
public:
   explicit VerifyUserUseCase(IUserRepository &userRepository,
                              IProjectRepositoryDB &projectRepositoryDB)
      : userRepository_(userRepository),
        projectRepositoryDB_(projectRepositoryDB) {}

   bool execute(const std::string &approverEmail,
                const std::string &approverPassword,
                const std::string &targetUserEmail) {

      // Para poder usarlos en el catch si algo truena
      auto approverOpt = userRepository_.findByEmail(approverEmail);
      auto targetOpt   = userRepository_.findByEmail(targetUserEmail);

      try {
         // Verificar que el usuario aprobador exista
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
            throw std::runtime_error("User " + approverEmail + " is not authorized to verify users");

         // Verificar que el usuario objetivo exista
         if (!targetOpt.has_value())
            throw std::runtime_error("Target user with email " + targetUserEmail + " does not exist");

         // Verificar que el status del usuario objetivo sea activo (esta trabajando actualmente)
         if (!userRepository_.isStatusActive(targetUserEmail))
            throw std::runtime_error("User: " + targetUserEmail + " is not active");

         // si ya esta verificado, no hacer nada
         if (userRepository_.isVerifiedUser(targetUserEmail))
            throw std::runtime_error("User " + targetUserEmail + " is already verified");

         // Cambiar el status del usuario objetivo
         bool ok = userRepository_.verifyUserEmail(targetUserEmail);

         // Registrar commit de éxito
         projectRepositoryDB_.addCommit(
            approverOpt->idUser,             // id del aprobador
            std::nullopt,                    // idFile nulo
            std::nullopt,                    // signature nula
            true,                            // isAccepted
            "VERIFY_USER",                   // command
            "User " + approverEmail +
               " verified user " + targetUserEmail // description
         );

         return ok;

      } catch (const std::exception &e) {
         // Intentar registrar commit de fallo.
         // Si no pudimos obtener al aprobador, usamos -1.
         int approverId = -1;
         if (approverOpt.has_value()) {
            approverId = approverOpt->idUser;
         }

         projectRepositoryDB_.addCommit(
            approverId,
            std::nullopt,
            std::nullopt,
            false,                           // isAccepted = false
            "VERIFY_USER",                   // command
            std::string("Failed to verify user ") + targetUserEmail +
               " by " + approverEmail + ": " + e.what()
         );

         throw; // re-lanzar para que la capa superior maneje el error
      }
   }

private:
   IUserRepository  &userRepository_;
   IProjectRepositoryDB &projectRepositoryDB_;
};
