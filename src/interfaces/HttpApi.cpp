#include <iostream>
#include "HttpApi.hpp"
#include "../third_party/json.hpp"

HttpApi::HttpApi(const char* certPath, const char* keyPath)
   : server_(certPath, keyPath) {
   // Constructor vacío o configuración inicial si necesitas
}

void HttpApi::registerRoutes(CreateRepositoryUseCase& createRepoUseCase) {

   /***********************************   INICIAR UN NUEVO REPOSITORIO  ***********************************/
   server_.Post("/repo/init",
      [&createRepoUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            // 1. Verificar que haya body
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }

            // 2. Parsear JSON del body
            nlohmann::json body = nlohmann::json::parse(req.body);

            // 3. Extraer campos "name" y "owner"
            if (!body.contains("repo_name") || !body.contains("owner_email")) {
               res.status = 400;
               res.set_content("Missing 'name' or 'email' field", "text/plain");
               return;
            }

            std::string repoName  = body["repo_name"].get<std::string>();
            std::string userEmail = body["owner_email"].get<std::string>();

            // (opcional) Validaciones simples
            if (repoName.empty() || userEmail.empty()) {
               res.status = 400;
               res.set_content("Fields 'repo_name' and 'owner_email' must not be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            Repository newRepo = createRepoUseCase.execute(repoName, userEmail);

            // 5. Construir respuesta JSON
            nlohmann::json responseBody;
            responseBody["status"] = "ok";
            responseBody["Repository_name"]   = newRepo.name;
            responseBody["Repository_owner"]  = newRepo.owner;

            res.status = 201; // Created
            res.set_content(responseBody.dump(), "application/json");
            std::cout << "Repository created: " << newRepo.name << " owned by " << newRepo.owner << std::endl << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            // Error al parsear JSON
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            // Error de negocio u otro tipo
            res.status = 500;
            std::cout << "Error creating repository: " << e.what() << std::endl << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );


   /***********************************   CLONAR UN REPOSITORIO  ***********************************/
   server_.Get("/repo/clone", [](const httplib::Request&, httplib::Response& res) {
      res.set_content("Repository cloned!", "text/plain");
   });



}

void HttpApi::listen(const char* host, int port) {
   std::cout << "Intentando iniciar servidor HTTPS en https://" << host << ":" << port << std::endl;
   std::cout << "Ahoa con json y msql!" << std::endl;
   
   // Verificar que los archivos de certificado y clave existen


   bool success = server_.listen(host, port);
   
   if (!success) {
      std::cerr << "Error: No se pudo iniciar el servidor en " << host << ":" << port << std::endl;
      std::cerr << "Posibles causas:" << std::endl;
      std::cerr << "  - Puerto ya en uso" << std::endl;
      std::cerr << "  - Certificados inválidos o no encontrados" << std::endl;
      std::cerr << "  - Permisos insuficientes" << std::endl;
      throw std::runtime_error("Failed to start server");
   }
}