#pragma once
#include <string>
#include <stdexcept>

// Caso de uso para usuarios en ls base de datos
#include "../domain/repositories/IUser.repository.hpp"

class SavePublicKeyRSAUseCase {
public:
   explicit SavePublicKeyRSAUseCase(IUserRepository &userRepository)
      : userRepository_(userRepository) {}

   bool execute(const std::string &email, const std::string &publicKey, const std::string &password) {
      
      // Verificar que el usuario exista
      auto existing = userRepository_.findByEmail(email);
      if (!existing.has_value())
         throw std::runtime_error("User with email " + email + " does not exist");

      // Validar que el status del usuario sea activo (esta trabajando actualmente)
      if (!userRepository_.isStatusActive(email))
         throw std::runtime_error("User: " + email + " is not active");

      // Verificar que el usuario esté verificado
      if (!userRepository_.isVerifiedUser(email))
         throw std::runtime_error("User with email " + email + " is not verified");

      // Verificar que el password sea correcto
      if (!userRepository_.isValidPassword(email, password))
         throw std::runtime_error("Invalid password for user: " + email);

      // Regla de negocio: solo se puede agregar si no hay clave RSA previa
      if (!userRepository_.notRSAKeyAdded(email))
         throw std::runtime_error("User with email " + email + " already has an RSA public key added");
         
      // Guardar la clave publica RSA
      return userRepository_.addPublicKeyRSA(email, publicKey);
   }
private:
   IUserRepository  &userRepository_;
};