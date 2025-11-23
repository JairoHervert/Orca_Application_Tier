#include <cstdlib>   // std::getenv
#include <stdexcept> // std::runtime_error
#include "ConfigEnv.hpp"
#include "../../third_party/dotenv.h"

namespace {
   std::string getEnvOrThrow(const char *name, const std::string &def = "") {
      const char *value = dotenv::getenv(name, def).c_str();
      if (!value) {
         throw std::runtime_error(std::string("Missing env variable: ") + name);
      }
      return std::string(value);
   }

   int getEnvIntOrThrow(const char *name, const std::string &def = "") {
      auto s = getEnvOrThrow(name, def);
      return std::stoi(s);
   }
}

ConfigEnv loadConfigFromEnv() {
   
   // Inicializar dotenv para cargar .env
   dotenv::init();
   
   // Crear estructura para guardar los env
   ConfigEnv cfg;

   // Obtener variables de entorno para iniciar el servidor
   cfg.serverHost = getEnvOrThrow("SERVER_HOST");
   cfg.serverPort = getEnvIntOrThrow("SERVER_PORT");

   // Rutas a los certificados SSL
   cfg.sslCertPath = getEnvOrThrow("SSL_CERT_PATH");
   cfg.sslKeyPath = getEnvOrThrow("SSL_KEY_PATH");

   cfg.repositoriesRoot = getEnvOrThrow("REPOSITORIES_ROOT");

   // Configuracion de la base de datos
   cfg.dbHost = getEnvOrThrow("DB_HOST");
   cfg.dbPort = getEnvIntOrThrow("DB_PORT");
   cfg.dbName = getEnvOrThrow("DB_NAME");
   cfg.dbUser = getEnvOrThrow("DB_USER");
   cfg.dbPassword = getEnvOrThrow("DB_PASSWORD");

   return cfg;
}
