#pragma once
#include <string>
#include <stdexcept>

// repositorios de operaciones con usuarios en la base de datos
#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"

class SavePublicKeyRSAUseCase {
public:
   explicit SavePublicKeyRSAUseCase(
      IUserRepository &userRepository,
      IProjectRepositoryDB &projectRepositoryDB)
      : userRepository_(userRepository),
        projectRepositoryDB_(projectRepositoryDB) {}

   bool execute(const std::string &email,
                const std::string &publicKey,
                const std::string &password) {
      
      // Para poder registrar commits incluso en caso de error
      std::optional<int> userIdOpt;

      try {
         // Verificar que el usuario exista
         auto existing = userRepository_.findByEmail(email);
         if (!existing.has_value())
            throw std::runtime_error("User with email " + email + " does not exist");

         userIdOpt = existing->idUser;

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
         bool ok = userRepository_.addPublicKeyRSA(email, publicKey);

         // Registrar commit de éxito
         projectRepositoryDB_.addCommit(
            userIdOpt.value_or(-1),
            std::nullopt,                  // idFile
            std::nullopt,                  // signature
            true,                          // isAccepted
            "SAVE_RSA_PUBLIC_KEY",         // command
            "RSA public key saved for user: " + email  // description
         );

         return ok;

      } catch (const std::exception &e) {
         // Registrar commit de fallo
         projectRepositoryDB_.addCommit(
            userIdOpt.value_or(-1),        // -1 si ni siquiera se pudo resolver el usuario
            std::nullopt,
            std::nullopt,
            false,                         // isAccepted
            "SAVE_RSA_PUBLIC_KEY",
            std::string("Failed to save RSA public key for user ") + email + ": " + e.what()
         );
         throw; // Propagar el error al llamador
      }
   }

private:
   IUserRepository  &userRepository_;
   IProjectRepositoryDB &projectRepositoryDB_;
};
