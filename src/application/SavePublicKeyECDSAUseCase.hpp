#pragma once
#include <string>
#include <stdexcept>

// Caso de uso para usuarios en ls base de datos
#include "../domain/entities/User.entity.hpp"
#include "../domain/repositories/IUser.repository.hpp"

class SavePublicKeyECDSAUseCase {
public:
   explicit SavePublicKeyECDSAUseCase(IUserRepository &userRepository)
      : userRepository_(userRepository) {}

   bool execute(const std::string &email, const std::string &publicKey, const std::string &password) {
      
      // Verificar que el usuario exista
      auto existing = userRepository_.findByEmail(email);
      if (!existing.has_value())
         throw std::runtime_error("User with email " + email + " does not exist");

      // Verificar que el password sea correcto
      if (!userRepository_.isValidPassword(email, password))
         throw std::runtime_error("Invalid password for user: " + email);

      // Regla de negocio: solo se puede agregar si no hay clave ECDSA previa
      if (!userRepository_.notECDSAKeyAdded(email))
         throw std::runtime_error("User with email " + email + " already has an ECDSA public key added");
         
      // Guardar la clave publica ECDSA
      return userRepository_.addPublicKeyToUser(email, publicKey);
   }
private:
   IUserRepository  &userRepository_;
};