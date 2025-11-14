#pragma once
#include <string>

struct ConfigEnv {

   // Configuracion del servidor
   std::string serverHost;
   int serverPort;

   // Certificados SSL
   std::string sslCertPath;
   std::string sslKeyPath;

   // std::string dbHost;
   // int dbPort;
   // std::string dbUser;
   // std::string dbPassword;
   // std::string dbName;
};

// Función que lee desde variables de entorno
ConfigEnv loadConfigFromEnv();