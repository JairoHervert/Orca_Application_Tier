// main.cpp
// #include "infrastructure/MariaDBRepositoryStore.hpp"
// #include "application/CreateRepositoryUseCase.hpp"

#include <iostream>
#include "infrastructure/config/ConfigEnv.hpp"
#include "interfaces/HttpApi.hpp"


int main() {
   try {
      // Cargar variables de entorno desde .env
      ConfigEnv configEnvs = loadConfigFromEnv();


      // 1. Infraestructura
      //  MariaDBRepositoryStore repoStore{/*config de conexión*/};

      // 2. Casos de uso (aplicación)
      //  CreateRepositoryUseCase createRepoUseCase{repoStore};

      // 2. Crear e inicializar API
      HttpApi http_api(configEnvs.sslCertPath.c_str(), configEnvs.sslKeyPath.c_str());
      http_api.registerRoutes();
      
      // 3. Iniciar servidor
      http_api.listen(configEnvs.serverHost.c_str(), configEnvs.serverPort);

   }
   catch (const std::exception &e) {
      // Manejo de errores inicialización
      // std::cerr << "Error during initialization: " << e.what() << std::endl;
      return 1;
   }
   return 0;
}
