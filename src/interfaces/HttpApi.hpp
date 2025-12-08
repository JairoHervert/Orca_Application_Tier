#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

// registrar los casos de uso necesarios
#include "../application/CreateRepositoryUseCase.hpp"
#include "../application/CreateUserUseCase.hpp"
#include "../application/SavePublicKeyECDSAUseCase.hpp"
#include "../application/ChangeLevelUserUseCase.hpp"
#include "../application/VerifyUserUseCase.hpp"
#include "../application/ChangeUserStatusUseCase.hpp"
#include "../application/SavePublicKeyRSAUseCase.hpp"
#include "../application/CipherRepositoryUseCase.hpp"
#include "../application/AddUserToRepoUseCase.hpp"
#include "../application/CloneRepositoryUseCase.hpp"
#include "../application/DecipherRepositoryUseCase.hpp"
#include "../application/HashFilesUseCase.hpp"
#include "../application/PushVerifyUseCase.hpp"
#include "../application/AddUserToFileUseCase.hpp"
#include "../application/CommitsListUseCase.hpp"
#include "../application/GetAESKeyUseCase.hpp"
#include "../application/ListAllProjectsUseCase.hpp"
#include "../application/ListEncryptedProjectsUseCase.hpp"
#include "../application/ListUserAccessibleProjectsUseCase.hpp"

/////////  caso de uso exclusivo para pruebas  //////////////////////
#include "../application/testUseCase.hpp"

class HttpApi {
public:
   // Constructor
   HttpApi(const char* certPath, const char* keyPath);

   // Registrar rutas para la API
   void registerRoutes(
      CreateRepositoryUseCase &createRepoUseCase,
      CreateUserUseCase &createUserUseCase,
      SavePublicKeyECDSAUseCase &saveKPubUseCase,
      ChangeLevelUserUseCase &changeLevelUserUseCase,
      VerifyUserUseCase &verifyUserUseCase,
      ChangeStatusUserUseCase &changeUserStatusUseCase,
      SavePublicKeyRSAUseCase &saveKPubRSAUseCase,
      CipherRepositoryUseCase &cipherRepoUseCase,
      AddUserToRepoUseCase &addUserToRepoUseCase,
      CloneRepositoryUseCase &cloneRepoUseCase,
      DecipherRepositoryUseCase &decipherRepoUseCase,
      HashFilesUseCase &hashRepoFilesCreate,
      PushVerifyUseCase &pushVerifyUseCase,
      AddUserToFileUseCase &addUserToFileUseCase,
      CommitsListUseCase &commitsListUseCase,
      GetAESKeyUseCase &getAESKeyUseCase,
      ListAllProjectsUseCase &listAllProjectsUseCase,
      ListEncryptedProjectsUseCase &listEncryptedProjectsUseCase,
      ListUserAccessibleProjectsUseCase &listUserAccessibleProjectsUseCase,


      TestUseCase &testUseCase  // Caso de uso exclusivo para pruebas
   );

   // Iniciar el servidor
   void listen(const char* host, int port);

private:
   httplib::SSLServer server_;
};