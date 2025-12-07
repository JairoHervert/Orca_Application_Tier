#pragma once
#include <string>
#include <stdexcept>

// repositorios de operaciones con usuarios en la base de datos
#include "../domain/repositories/IUser.repository.hpp"
// repositorio de commits / proyectos
#include "../domain/repositories/IProjectDB.repository.hpp"

class SavePublicKeyECDSAUseCase {
public:
   explicit SavePublicKeyECDSAUseCase(
      IUserRepository &userRepository,
      IProjectRepositoryDB &projectRepositoryDB)
      : userRepository_(userRepository),
        projectRepositoryDB_(projectRepositoryDB) {}

   bool execute(const std::string &email,
                const std::string &publicKey,
                const std::string &password) {

      // Nota: usamos try/catch para registrar commit tanto en éxito como en fallo
      try {
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

         // Regla de negocio: solo se puede agregar si no hay clave ECDSA previa
         if (!userRepository_.notECDSAKeyAdded(email))
            throw std::runtime_error("User with email " + email + " already has an ECDSA public key added");
         
         // Guardar la clave publica ECDSA
         bool ok = userRepository_.addPublicKeyECDSA(email, publicKey);

         // Commit de éxito
         projectRepositoryDB_.addCommit(
            existing->idUser,          // idUser
            std::nullopt,             // idFile (no aplica)
            std::nullopt,             // signature (no aplica)
            true,                     // isAccepted
            "SAVE_ECDSA_PUBLIC_KEY",  // command
            "ECDSA public key saved for user " + email
         );

         return ok;

      } catch (const std::exception &e) {
         // En fallo intentamos recuperar el idUser si es posible
         int userId = -1;
         auto existing = userRepository_.findByEmail(email);
         if (existing.has_value()) {
            userId = existing->idUser;
         }

         projectRepositoryDB_.addCommit(
            userId,                   // -1 si no se pudo resolver el usuario
            std::nullopt,             // idFile (no aplica)
            std::nullopt,             // signature (no aplica)
            false,                    // isAccepted
            "SAVE_ECDSA_PUBLIC_KEY",  // command
            std::string("Failed to save ECDSA public key for user ") + email +
               ": " + e.what()
         );

         throw; // repropagar la excepción al llamador
      }
   }

private:
   IUserRepository        &userRepository_;
   IProjectRepositoryDB   &projectRepositoryDB_;
};
