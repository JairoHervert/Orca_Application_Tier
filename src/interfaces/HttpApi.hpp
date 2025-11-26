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


      TestUseCase &testUseCase  // Caso de uso exclusivo para pruebas
   );

   // Iniciar el servidor
   void listen(const char* host, int port);

private:
   httplib::SSLServer server_;
};