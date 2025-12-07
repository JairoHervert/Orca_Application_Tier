#pragma once
#include <string>
#include <stdexcept>

// repositorios de operaciones con usuarios en la base de datos
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"

class ChangeLevelUserUseCase {
public:
   explicit ChangeLevelUserUseCase(
      IUserRepository &userRepository,
      IProjectRepositoryDB &projectRepositoryDB)
      : userRepository_(userRepository),
        projectRepositoryDB_(projectRepositoryDB) {}

   bool execute(const std::string &approverEmail,
                const std::string &approverPassword,
                const std::string &targetUserEmail,
                int newRole) {

      // Obtener (si existen) para poder registrar commits en caso de error
      auto approverOpt = userRepository_.findByEmail(approverEmail);
      auto targetOpt   = userRepository_.findByEmail(targetUserEmail);

      try {
         // 0. Validar que el nuevo rol sea válido
         if (newRole < 1 || newRole > 3) {
            throw std::runtime_error(
               "Invalid role value: " + std::to_string(newRole) +
               ". Must be 1 (Developer), 2 (Leader), or 3 (Senior)"
            );
         }

         // 1. Verificar que el usuario aprobador exista
         if (!approverOpt.has_value())
            throw std::runtime_error("Approver user with email " + approverEmail + " does not exist");

         // 2. Verificar que el password del aprobador sea correcto
         if (!userRepository_.isValidPassword(approverEmail, approverPassword))
            throw std::runtime_error("Invalid password for approver user: " + approverEmail);

         // 3. Verificar que el status del usuario aprobador sea activo
         if (!userRepository_.isStatusActive(approverEmail))
            throw std::runtime_error("User: " + approverEmail + " is not active");

         // 4. Verificar que el usuario aprobador esté verificado
         if (!userRepository_.isVerifiedUser(approverEmail))
            throw std::runtime_error("Approver user with email " + approverEmail + " is not verified");

         // 5. Verificar que el usuario aprobador tenga permisos (Senior)
         if (!userRepository_.isSeniorUser(approverEmail))
            throw std::runtime_error("User " + approverEmail + " is not authorized to change user levels");

         // 6. Verificar que el usuario objetivo exista
         if (!targetOpt.has_value())
            throw std::runtime_error("Target user with email " + targetUserEmail + " does not exist");

         // 7. Verificar que el status del usuario objetivo sea activo
         if (!userRepository_.isStatusActive(targetUserEmail))
            throw std::runtime_error("User: " + targetUserEmail + " is not active");

         // 8. Verificar que el usuario objetivo esté verificado
         if (!userRepository_.isVerifiedUser(targetUserEmail))
            throw std::runtime_error("Target user with email " + targetUserEmail + " is not verified");

         // 9. Cambiar el nivel del usuario objetivo
         bool changed = userRepository_.changeLevelUser(targetUserEmail, newRole);

         // 10. Construir texto de rol para el commit
         std::string roleStr;
         switch (newRole) {
            case 1: roleStr = "Developer"; break;
            case 2: roleStr = "Leader";    break;
            case 3: roleStr = "Senior";    break;
            default:
               roleStr = "Unknown(" + std::to_string(newRole) + ")";
               break;
         }

         // 11. Registrar commit de éxito
         projectRepositoryDB_.addCommit(
            approverOpt->idUser,      // id del aprobador
            std::nullopt,             // sin archivo asociado
            std::nullopt,             // sin firma digital
            true,                     // isAccepted = true
            "CHANGE_USER_LEVEL",      // comando
            "User " + approverEmail +
               " changed level of " + targetUserEmail +
               " to " + roleStr + " (" + std::to_string(newRole) + ")"
         );

         return changed;

      } catch (const std::exception &e) {
         // Registrar commit de fallo
         int approverId = approverOpt.has_value() ? approverOpt->idUser : -1;

         projectRepositoryDB_.addCommit(
            approverId,
            std::nullopt, // sin archivo
            std::nullopt, // sin firma
            false,        // isAccepted = false
            "CHANGE_USER_LEVEL",
            std::string("Failed to change level of ") + targetUserEmail +
               " by " + approverEmail + ": " + e.what()
         );

         throw; // se propaga al HttpApi para que devuelva 4xx/5xx
      }
   }

private:
   IUserRepository        &userRepository_;
   IProjectRepositoryDB   &projectRepositoryDB_;
};
