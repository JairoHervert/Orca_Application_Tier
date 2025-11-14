#pragma once
#include <string>

struct ConfigEnv {
   std::string serverHost;
   int serverPort;

   // std::string dbHost;
   // int dbPort;
   // std::string dbUser;
   // std::string dbPassword;
   // std::string dbName;
};

// Función que lee desde variables de entorno
ConfigEnv loadConfigFromEnv();