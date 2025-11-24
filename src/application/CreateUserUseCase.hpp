#pragma once
#include <string>
#include <stdexcept>

// Caso de uso para usuarios en ls base de datos
// #include "../domain/entities/User.entity.hpp"
#include "../domain/repositories/IUser.repository.hpp"

// Helper para validar formato de email
#include "../domain/utils/EmailValidator.hpp"

class CreateUserUseCase {
public:
   explicit CreateUserUseCase(IUserRepository &userRepository)
      : userRepository_(userRepository) {}

   bool execute(const std::string &name, const std::string &email, const std::string &password) {
      
      // Validar formato de email
      if (!isValidEmailFormat(email)) {
         throw std::runtime_error("Invalid email format: " + email);
      }
      
      // Regla de negocio: no permitir duplicados
      auto existing = userRepository_.findByEmail(email);
      if (existing.has_value()) {
         throw std::runtime_error("User with email " + email + " already exists");
      }
      return userRepository_.create(name, email, password);
   }

private:
   IUserRepository  &userRepository_;
};