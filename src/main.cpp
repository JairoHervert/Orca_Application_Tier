// g++ src/main.cpp src/infrastructure/config/ConfigEnv.cpp src/interfaces/HttpApi.cpp -I../third_party -I/usr/include/mysql -o main -lssl -lcrypto -lsoci_core -lsoci_mysql -lmariadb

#include <iostream>
#include "infrastructure/config/ConfigEnv.hpp"
#include "interfaces/HttpApi.hpp"

#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>

// Repositorios
#include "infrastructure/database/DBUserRepository.hpp"
#include "infrastructure/storage/FilesystemStorage.hpp"
#include "infrastructure/database/DBProjectRepository.hpp"

// Casos de uso
#include "application/CreateRepositoryUseCase.hpp"
#include "application/CreateUserUseCase.hpp"
#include "application/SavePublicKeyECDSAUseCase.hpp"


int main() {
   try {
      // 1. Cargar variables de entorno desde .env
      ConfigEnv configEnvs = loadConfigFromEnv();

      // 2. Crear sesion SOCI (conexion a la BDD MySQL/MariaDB)
      std::string connStr =
         "db=" + configEnvs.dbName +
         " user=" + configEnvs.dbUser +
         " password=" + configEnvs.dbPassword +
         " host=" + configEnvs.dbHost;

      soci::session sql(soci::mysql, connStr);

      // 3. Infraestructura para repositorios
      FilesystemStorage repoStore{configEnvs.repositoriesRoot};
      DBUserRepository userRepo{sql};
      DBProjectRepository projectRepo{sql};

      // 4. Casos de uso (aplicacion)
      CreateRepositoryUseCase createRepoUseCase{repoStore, userRepo, projectRepo};
      CreateUserUseCase createUserUseCase{userRepo};
      SavePublicKeyECDSAUseCase saveKPubUseCase{userRepo};

      // 5. Crear e inicializar API HTTP con SSL
      HttpApi http_api(configEnvs.sslCertPath.c_str(), configEnvs.sslKeyPath.c_str());

      // 6. Registrar rutas e inyectar casos de uso donde se necesite
      http_api.registerRoutes(
         createRepoUseCase,
         createUserUseCase,
         saveKPubUseCase
      );
      
      // 7. Iniciar servidor
      http_api.listen(configEnvs.serverHost.c_str(), configEnvs.serverPort);

   }
   catch (const std::exception &e) {
      // Manejo de errores inicialización
      // std::cerr << "Error during initialization: " << e.what() << std::endl;
      return 1;
   }
   return 0;
}
