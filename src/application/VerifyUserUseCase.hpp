#pragma once
#include <string>
#include <stdexcept>

// repositorios de operaciones con usuarios en la base de datos
#include "../domain/repositories/IUser.repository.hpp"

class VerifyUserUseCase {
public:
   explicit VerifyUserUseCase(IUserRepository &userRepository)
      : userRepository_(userRepository) {}

   bool execute(const std::string &approverEmail, const std::string &approverPassword, const std::string &targetUserEmail) {

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
         throw std::runtime_error("User " + approverEmail + " is not authorized to verify users");


      // Verificar que el usuario objetivo exista
      if (!userRepository_.findByEmail(targetUserEmail).has_value())
         throw std::runtime_error("Target user with email " + targetUserEmail + " does not exist");

      // Verificar que el status del usuario objetivo sea activo (esta trabajando actualmente)
      if (!userRepository_.isStatusActive(targetUserEmail))
         throw std::runtime_error("User: " + targetUserEmail + " is not active");

      // Cambiar el status del usuario objetivo
      return userRepository_.verifyUserEmail(targetUserEmail);
   }
private:
   IUserRepository  &userRepository_;
};