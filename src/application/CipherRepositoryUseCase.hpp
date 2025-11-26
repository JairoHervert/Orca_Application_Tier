#pragma once
#include <iostream>
#include <string>
#include <stdexcept>

// Repo de cripto, storage, usuaros DB
#include "../domain/repositories/IProtectRepoCrypto.repository.hpp"
#include "../domain/repositories/IRepositoryStore.repository.hpp"
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/repositories/IUser.repository.hpp"


class CipherRepositoryUseCase {
public:
   explicit CipherRepositoryUseCase(IRepositoryStore &repositoryStore,
                                    IProjectRepositoryDB &DBProjectRepository,
                                    IUserRepository &userRepository,
                                    IProtectRepoCryptoRepository &cryptoRepo)
      : repositoryStore_(repositoryStore),
        DBProjectRepository(DBProjectRepository),
        userRepository_(userRepository),
        cryptoRepo_(cryptoRepo) {}
        
   std::string execute(const std::string &leaderEmail, const std::string &leaderPassword, const std::string &seniorEmail, const std::string &repoName, const std::string &projectAlias) {
     
      /******************  Verificar existencias de los actores ******************/

      // 1. Verificar que el usuario líder exista
      auto leaderOpt = userRepository_.findByEmail(leaderEmail);
      if (!leaderOpt.has_value())
         throw std::runtime_error("Leader user with email " + leaderEmail + " does not exist");

      // 2. Verificar que el repositorio exista en DB y en storage
      auto projectOpt = DBProjectRepository.findByName(repoName);
      if (!projectOpt.has_value())
         throw std::runtime_error("Repository with name " + repoName + " does not exist in DB");

      if (!repositoryStore_.findByName(repoName).has_value())
         throw std::runtime_error("Repository with name " + repoName + " does not exist in storage");
      
      // 3. Verificar que el usuario senior exista
      auto seniorOpt = userRepository_.findByEmail(seniorEmail);
      if (!seniorOpt.has_value())
         throw std::runtime_error("Senior user with email " + seniorEmail + " does not exist");



      /******************  Verificar al usuario líder  ******************/

      // Pasar a User la info del líder
      User leaderUser = leaderOpt.value();

      // 4. Verificar que el usuario esté verificado y activo
      if (leaderUser.verify == 0 || leaderUser.status == 0)
         throw std::runtime_error("Leader user with email " + leaderEmail + " is not verified or not active");

      // 5. Verificar que es líder
      if (leaderUser.role != 2)
         throw std::runtime_error("User " + leaderEmail + " is not authorized to cipher repositories");

      // 6. Verificar que el password del líder sea correcto
      if (!userRepository_.isValidPassword(leaderEmail, leaderPassword))
         throw std::runtime_error("Invalid password for leader user: " + leaderEmail);

      // 7. Verificar que el lider sea el owner del repo que solicita cifrar
      Repository repo = projectOpt.value();
      if (repo.ownerId != leaderUser.idUser)
         throw std::runtime_error("User " + leaderEmail + " is not the leader of the repository " + repoName);


      
      /******************  Verificar al usuario senior  ******************/
      // 8. Verificar que el usuario senior tenga permisos
      if (seniorOpt->role != 3)
         throw std::runtime_error("User " + seniorEmail + " is not authorized as senior to cipher repositories");

      // 9. Verificar que el usuario senior esté verificado y activo
      if (seniorOpt->verify == 0 || seniorOpt->status == 0)
         throw std::runtime_error("Senior user with email " + seniorEmail + " is not verified or not active");


      /****************** Existencia de claves públicas RSA ******************/
      // 10. Verificar que el líder tenga clave pública RSA
      if (userRepository_.notRSAKeyAdded(leaderEmail))
         throw std::runtime_error("Leader user with email " + leaderEmail + " does not have a RSA public key added");

      // 11. Verificar que el senior tenga clave pública RSA
      if (userRepository_.notRSAKeyAdded(seniorEmail))
         throw std::runtime_error("Senior user with email " + seniorEmail + " does not have a RSA public key added");



      /******************  Cifrado del repo  ******************/

      // 12. Verificar que el repo no esté ya cifrado (comprobando en el registro de la base de datos)
      if (DBProjectRepository.existsRepoAlias(repoName + "_" + projectAlias))
         throw std::runtime_error("The repository alias " + repoName + "_" + projectAlias + " for the repository " + repoName + " already exists in the database. Choose another alias.");
         
      // 13. Generar clave AES
      std::string aesKeyB64 = cryptoRepo_.gen_b64_AES_GCM_Key();
      
      // 14. Cifrar la clave AES con la clave pública RSA del usuario lider del repo, aun no la guarda en DB
      std::string aesKeyCifradaRSA_Leader = cryptoRepo_.cipher_RSA_OAEP(aesKeyB64, leaderUser.publicKeyRSA.c_str());

      // 15. Cifrar la clave AES con la clave pública RSA del usuario senior, aun no la guarda en DB
      std::string aesKeyCifradaRSA_Senior = cryptoRepo_.cipher_RSA_OAEP(aesKeyB64, seniorOpt->publicKeyRSA.c_str());

      // 16. Crear tar del repo
      std::filesystem::path tarPath = repositoryStore_.folderToTar(repoName, projectAlias);

      // 17. Cifrar el tar → fileOutPath en carpeta de cifrado
      std::string cipherTarPath = tarPath.string() + ".enc";
      bool cifradoOk = cryptoRepo_.cipher_AES_GCM(tarPath.string(), cipherTarPath, aesKeyB64);

      // 18. Eliminar el tar original (se cifre o no correctamente, no se necesita más)
      bool tarDeleted = repositoryStore_.deleteCipherFile((tarPath.filename()).string());
      if (!tarDeleted) throw std::runtime_error("Error deleting the original tar file: " + tarPath.string());


      // 19. Verificar que el cifrado fue correcto
      if (!cifradoOk) throw std::runtime_error("Error ciphering the repository tar file: " + tarPath.string());


      // 20. Si el cifrado fue correcto, guardar las claves cifradas en la tabla repo_protect
      bool passwordStored_Leader = DBProjectRepository.addPassword_repo_user(leaderUser.idUser, repo.idProject, aesKeyCifradaRSA_Leader, repoName + "_" + projectAlias);
      if (!passwordStored_Leader) {
         repositoryStore_.deleteCipherFile((tarPath.filename().string() + ".enc"));
         throw std::runtime_error("Error storing the ciphered AES key for leader user in DB");
      }
      
      bool passwordStored_Senior = DBProjectRepository.addPassword_repo_user(seniorOpt->idUser, repo.idProject, aesKeyCifradaRSA_Senior, repoName + "_" + projectAlias);
      if (!passwordStored_Senior) {
         repositoryStore_.deleteCipherFile((tarPath.filename().string() + ".enc"));
         throw std::runtime_error("Error storing the ciphered AES key for senior user in DB");
      }

      // 21. Retornar la clave AES cifrada con RSA del líder para mostrar al cliente (lider/owner del repo/proyecto)
      return aesKeyCifradaRSA_Leader;
   }

private:
   IRepositoryStore  &repositoryStore_;
   IProjectRepositoryDB &DBProjectRepository;
   IUserRepository  &userRepository_;
   IProtectRepoCryptoRepository &cryptoRepo_;
};