// main.cpp
// #include "infrastructure/MariaDBRepositoryStore.hpp"
// #include "application/CreateRepositoryUseCase.hpp"

#include <iostream>
#include "infrastructure/config/ConfigEnv.hpp"
// #include "interfaces/HttpApi.hpp"
#include <httplib.h>


int main() {
   try {
      // Cargar variables de entorno desde .env
      ConfigEnv config = loadConfigFromEnv();


      // 1. Infraestructura
      //  MariaDBRepositoryStore repoStore{/*config de conexión*/};

      // 2. Casos de uso (aplicación)
      //  CreateRepositoryUseCase createRepoUseCase{repoStore};


      httplib::Server server;
      server.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
         res.set_content("Hello World!", "text/plain");
      });

      std::cout << "Servidor HTTPS en " << config.serverHost << ":" << config.serverPort << std::endl;
      server.listen(config.serverHost, config.serverPort);

   }
   catch (const std::exception &e) {
      // Manejo de errores inicialización
      // std::cerr << "Error during initialization: " << e.what() << std::endl;
      return 1;
   }
   return 0;
}
