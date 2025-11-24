#pragma once
#include <string>
#include <stdexcept>

// repositorios de operaciones con usuarios en la base de datos
#include "../domain/repositories/IUser.repository.hpp"

class ChangeStatusUserUseCase {
public:
   explicit ChangeStatusUserUseCase(IUserRepository &userRepository)
      : userRepository_(userRepository) {}

   bool execute(const std::string &approverEmail, const std::string &approverPassword, const std::string &targetUserEmail, int newStatus) {

      // Verificar que el nuevo status sea valido (0: no trabaja actualmente, 1: trabaja actualmente)
      if (newStatus < 0 || newStatus > 1)
         throw std::runtime_error("Invalid status value: " + std::to_string(newStatus) + ". Must be 0 (Inactive - working) or 1 (Active - not working)");

      // Verificar que el usuario aprobador exista
      if (!userRepository_.findByEmail(approverEmail).has_value())
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
      if (!userRepository_.findByEmail(targetUserEmail).has_value())
         throw std::runtime_error("Target user with email " + targetUserEmail + " does not exist");

      // Cambiar el status del usuario objetivo
      return userRepository_.changeActiveStatus(targetUserEmail, newStatus);
   }
private:
   IUserRepository  &userRepository_;
};