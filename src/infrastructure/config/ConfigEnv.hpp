#pragma once
#include <string>

struct ConfigEnv {

   // Configuracion del servidor
   std::string serverHost;
   int serverPort;

   // Certificados SSL
   std::string sslCertPath;
   std::string sslKeyPath;

   // para las pruebas locales de guardado de repositorios
   std::string repositoriesRoot;

   // Configuracion de la base de datos
   std::string dbHost;
   int dbPort;
   std::string dbName;
   std::string dbUser;
   std::string dbPassword;
};

// Función que lee desde variables de entorno
ConfigEnv loadConfigFromEnv();