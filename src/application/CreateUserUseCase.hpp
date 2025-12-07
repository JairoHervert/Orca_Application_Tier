#pragma once
#include <string>
#include <stdexcept>
#include <optional>

#include "../domain/repositories/IUser.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/utils/EmailValidator.hpp"

class CreateUserUseCase {
public:
   explicit CreateUserUseCase(
      IUserRepository &userRepository,
      IProjectRepositoryDB &projectRepositoryDB)
      : userRepository_(userRepository),
        projectRepositoryDB_(projectRepositoryDB) {}

   bool execute(const std::string &name,
                const std::string &email,
                const std::string &password) {

      // Para poder usarlo también en el catch
      std::optional<User> userForLog;

      try {
         // 1. Validar formato de email
         if (!isValidEmailFormat(email)) {
            throw std::runtime_error("Invalid email format: " + email);
         }

         // 2. No permitir duplicados
         auto existing = userRepository_.findByEmail(email);
         if (existing.has_value()) {
            // Para el commit de fallo ya sabemos quién es
            userForLog = existing;
            throw std::runtime_error("User with email " + email + " already exists");
         }

         // 3. Crear el usuario en la BDD
         bool created = userRepository_.create(name, email, password);

         // Intentar recuperar el usuario recién creado para obtener su id
         auto newUserOpt = userRepository_.findByEmail(email);
         int newUserId = -1;
         if (newUserOpt.has_value()) {
            newUserId = newUserOpt->idUser;
            userForLog = newUserOpt;
         }

         // 4. Registrar commit de éxito
         projectRepositoryDB_.addCommit(
            newUserId,              // idUser (si no se encontró, será -1 pero el usuario debería existir)
            std::nullopt,           // idFile
            std::nullopt,           // signature
            true,                   // isAccepted
            "CREATE_USER",          // command
            "User " + email + " created successfully"
         );

         return created;

      } catch (const std::exception &e) {
         // Intentar asociar el commit a un usuario real si ya existe
         int userIdForLog = -1;
         if (!userForLog.has_value()) {
            auto existingAgain = userRepository_.findByEmail(email);
            if (existingAgain.has_value()) {
               userIdForLog = existingAgain->idUser;
               userForLog   = existingAgain;
            }
         } else {
            userIdForLog = userForLog->idUser;
         }

         // Registrar commit de fallo
         projectRepositoryDB_.addCommit(
            userIdForLog,                  // si no se conoce, será -1
            std::nullopt,                  // idFile
            std::nullopt,                  // signature
            false,                         // isAccepted
            "CREATE_USER",                 // command
            std::string("Failed to create user: ") + e.what()
         );

         throw; // Propagar el error a la capa superior (HttpApi)
      }
   }

private:
   IUserRepository       &userRepository_;
   IProjectRepositoryDB  &projectRepositoryDB_;
};
