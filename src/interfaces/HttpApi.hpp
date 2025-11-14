#pragma once
#include <httplib.h>

class HttpApi {
public:
   HttpApi();

   // Registras rutas usando los casos de uso necesarios
   // void registerRoutes(CreateRepositoryUseCase& createRepoUseCase);

   // Iniciar el servidor
   void listen(const char* host, int port);

private:
   httplib::Server server_;
};
