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
#include "infrastructure/crypto/ProtectRepo.hpp"
#include "infrastructure/crypto/PushRepoVerifyCrypto.hpp"

// Casos de uso
#include "application/CreateRepositoryUseCase.hpp"
#include "application/CreateUserUseCase.hpp"
#include "application/SavePublicKeyECDSAUseCase.hpp"
#include "application/ChangeLevelUserUseCase.hpp"
#include "application/VerifyUserUseCase.hpp"
#include "application/ChangeUserStatusUseCase.hpp"
#include "application/SavePublicKeyRSAUseCase.hpp"
#include "application/CipherRepositoryUseCase.hpp"
#include "application/AddUserToRepoUseCase.hpp"
#include "application/CloneRepositoryUseCase.hpp"
#include "application/DecipherRepositoryUseCase.hpp"
#include "application/HashFilesUseCase.hpp"
#include "application/PushVerifyUseCase.hpp"
#include "application/AddUserToFileUseCase.hpp"
#include "application/CommitsListUseCase.hpp"
#include "application/GetAESKeyUseCase.hpp"
#include "application/ListAllProjectsUseCase.hpp"

//////////////// Caso de uso exclusivo para pruebas ////////////////////////
#include "application/testUseCase.hpp"


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
      FilesystemStorage repoStore{configEnvs.repositoriesRoot, configEnvs.repositoriesCipher, configEnvs.repositoriesWorkspace};
      DBUserRepository userRepo{sql};
      DBProjectRepository projectRepoDB{sql};
      ProtectRepoCrypto repoCrypto{};
      PushRepoVerifyCrypto pushVerifyCrypto{};

      // 4. Casos de uso (aplicacion)
      CreateRepositoryUseCase createRepoUseCase{repoStore, userRepo, projectRepoDB};
      CreateUserUseCase createUserUseCase{userRepo, projectRepoDB};
      SavePublicKeyECDSAUseCase saveKPubUseCase{userRepo, projectRepoDB};
      ChangeLevelUserUseCase changeLevelUserUseCase{userRepo, projectRepoDB};
      VerifyUserUseCase verifyUserUseCase{userRepo, projectRepoDB};
      ChangeStatusUserUseCase changeUserStatusUseCase{userRepo, projectRepoDB};
      SavePublicKeyRSAUseCase saveKPubRSAUseCase{userRepo, projectRepoDB};
      CipherRepositoryUseCase cipherRepoUseCase{repoStore, projectRepoDB, userRepo, repoCrypto};
      AddUserToRepoUseCase addUserToRepoUseCase{projectRepoDB, userRepo};
      CloneRepositoryUseCase cloneRepoUseCase{repoStore, userRepo, projectRepoDB};
      DecipherRepositoryUseCase decipherRepoUseCase{repoStore, userRepo, projectRepoDB};
      HashFilesUseCase hashRepoFilesCreate{repoStore, projectRepoDB, userRepo, pushVerifyCrypto};
      PushVerifyUseCase pushVerifyUseCase{repoStore, projectRepoDB, userRepo, pushVerifyCrypto};
      AddUserToFileUseCase addUserToFileUseCase{projectRepoDB, userRepo};
      CommitsListUseCase commitsListUseCase{projectRepoDB};
      GetAESKeyUseCase getAESKeyUseCase{projectRepoDB, userRepo};
      ListAllProjectsUseCase listAllProjectsUseCase{projectRepoDB};

      ////////////////// Caso de uso exclusivo para pruebas ////////////////////////
      TestUseCase testUseCase{repoStore, repoCrypto};

      // 5. Crear e inicializar API HTTP con SSL
      HttpApi http_api(configEnvs.sslCertPath.c_str(), configEnvs.sslKeyPath.c_str());

      // 6. Registrar rutas e inyectar casos de uso donde se necesite
      http_api.registerRoutes( 
         createRepoUseCase,
         createUserUseCase,
         saveKPubUseCase,
         changeLevelUserUseCase,
         verifyUserUseCase,
         changeUserStatusUseCase,
         saveKPubRSAUseCase,
         cipherRepoUseCase,
         addUserToRepoUseCase,
         cloneRepoUseCase,
         decipherRepoUseCase,
         hashRepoFilesCreate,
         pushVerifyUseCase,
         addUserToFileUseCase,
         commitsListUseCase,
         getAESKeyUseCase,
         listAllProjectsUseCase,

         testUseCase  // Caso de uso exclusivo para pruebas
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
