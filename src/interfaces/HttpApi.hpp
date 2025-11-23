#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

// registrar los casos de uso necesarios
#include "../application/CreateRepositoryUseCase.hpp"

class HttpApi {
public:
   // Constructor
   HttpApi(const char* certPath, const char* keyPath);

   // Registrar rutas para la API
   void registerRoutes(CreateRepositoryUseCase &createRepoUseCase);

   // Iniciar el servidor
   void listen(const char* host, int port);

private:
   httplib::SSLServer server_;
};