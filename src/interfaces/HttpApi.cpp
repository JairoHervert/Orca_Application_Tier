#include <iostream>
#include "HttpApi.hpp"
#include "../third_party/json.hpp"

HttpApi::HttpApi(const char* certPath, const char* keyPath)
   : server_(certPath, keyPath) {
   // Constructor vacío o configuración inicial si necesitas
}

void HttpApi::registerRoutes(
   CreateRepositoryUseCase& createRepoUseCase,
   CreateUserUseCase& createUserUseCase,
   SavePublicKeyECDSAUseCase& saveKPubUseCase,
   ChangeLevelUserUseCase& changeLevelUserUseCase,
   VerifyUserUseCase& verifyUserUseCase,
   ChangeStatusUserUseCase& changeUserStatusUseCase
) {

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

            // 3. Extraer campos "repo_name" y "owner_email" y "owner_password"
            if (!body.contains("repo_name") || !body.contains("owner_email") || !body.contains("owner_password")) {
               res.status = 400;
               res.set_content("Missing 'name' or 'email' field", "text/plain");
               return;
            }

            std::string repoName  = body["repo_name"].get<std::string>();
            std::string userEmail = body["owner_email"].get<std::string>();
            std::string userPassword = body["owner_password"].get<std::string>();

            // (opcional) Validaciones simples
            if (repoName.empty() || userEmail.empty() || userPassword.empty()) {
               res.status = 400;
               res.set_content("Fields 'repo_name', 'owner_email' and 'owner_password' cannot be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            Repository newRepo = createRepoUseCase.execute(repoName, userEmail, userPassword);

            // 5. Construir respuesta JSON
            nlohmann::json responseBody;
            responseBody["status"] = "ok";
            responseBody["Repository_name"]   = newRepo.name;
            // responseBody["Repository_owner"]  = newRepo.owner;

            res.status = 201; // Created
            res.set_content(responseBody.dump(), "application/json");
            std::cout << "Repository created: " << newRepo.name << std::endl << std::endl;
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


   /***********************************   DAR DE ALTA NUEVO USER  ***********************************/
   server_.Post("/user/create",
      [&createUserUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            // 1. Verificar que haya body
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }

            // 2. Parsear JSON del body
            nlohmann::json body = nlohmann::json::parse(req.body);

            // 3. Extraer campos "name", "email" y "password"
            if (!body.contains("name") || !body.contains("email") || !body.contains("password")) {
               res.status = 400;
               res.set_content("Missing 'name', 'email' or 'password' field", "text/plain");
               return;
            }

            std::string name     = body["name"].get<std::string>();
            std::string email    = body["email"].get<std::string>();
            std::string password = body["password"].get<std::string>();

            // (opcional) Validaciones simples
            if (name.empty() || email.empty() || password.empty()) {
               res.status = 400;
               res.set_content("Fields 'name', 'email' and 'password' cannot be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            bool created = createUserUseCase.execute(name, email, password);
            
            nlohmann::json responseBody;

            if (!created) {
               res.status = 500;
               responseBody["status"] = "error";
               responseBody["message"] = "User could not be created";
               res.set_content(responseBody.dump(), "application/json");
               std::cout << "Failed to create user: " << name << " with email " << email << std::endl << std::endl;
               return;
            }

            // 5. Construir respuesta JSON
            responseBody["status"] = "ok";
            responseBody["user_name"]  = name;
            responseBody["user_email"] = email;

            res.status = 201; // Created
            res.set_content(responseBody.dump(), "application/json");
            std::cout << "User created: " << name << " with email " << email << std::endl << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            // Error al parsear JSON
            res.status = 400;
            std::cout << "JSON parse error: " << e.what() << std::endl;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            // Error de negocio u otro tipo
            res.status = 500;
            std::cout << "Error creating user: " << e.what() << std::endl << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
         catch (...) {
            // Capturar cualquier otro tipo de excepción
            res.status = 500;
            std::cout << "Unknown error occurred while creating user." << std::endl << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
         }
      }
   );


   /***********************************   INSERTAR K_PUB ECDSA A UN USUARIO  ***********************************/
   server_.Post("/user/add_kpub_ecdsa",
      [&saveKPubUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            // 1. Verificar que haya body
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }

            // 2. Parsear JSON del body
            nlohmann::json body = nlohmann::json::parse(req.body);

            // 3. Extraer campos "email", "password" y "public_key"
            if (!body.contains("email") || !body.contains("password") || !body.contains("kpub_ecdsa")) {
               res.status = 400;
               res.set_content("Missing 'email', 'password' or 'public_key' field", "text/plain");
               return;
            }

            std::string email    = body["email"].get<std::string>();
            std::string password = body["password"].get<std::string>();
            std::string publicKey = body["kpub_ecdsa"].get<std::string>();

            // (opcional) Validaciones simples
            if (email.empty() || password.empty() || publicKey.empty()) {
               res.status = 400;
               res.set_content("Fields 'name', 'email' and 'password' cannot be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            bool keySaved = saveKPubUseCase.execute(email, publicKey, password);

            // 5. Construir respuesta JSON
            nlohmann::json responseBody;
            responseBody["status"] = "ok";
            responseBody["user_email"] = email;
            responseBody["key_saved"]  = keySaved;

            res.status = 201; // Created
            res.set_content(responseBody.dump(), "application/json");
            std::cout << "Public key saved for user with email " << email << std::endl << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            // Error al parsear JSON
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            // Error de negocio u otro tipo
            res.status = 500;
            std::cout << "Error saving public key: " << e.what() << std::endl << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );



   /***********************************   CAMBIAR EL ROL A UN USUARIO  ***********************************/
   server_.Post("/user/change_level",
      [&changeLevelUserUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            // 1. Verificar que haya body
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }

            // 2. Parsear JSON del body
            nlohmann::json body = nlohmann::json::parse(req.body);

            // 3. Extraer campos necesarios
            if (!body.contains("approver_email") || !body.contains("approver_password") || !body.contains("target_user_email") || !body.contains("new_role")) {
               res.status = 400;
               res.set_content("Missing required fields", "text/plain");
               return;
            }

            std::string approverEmail    = body["approver_email"].get<std::string>();
            std::string approverPassword = body["approver_password"].get<std::string>();
            std::string targetUserEmail  = body["target_user_email"].get<std::string>();
            int newRole                  = body["new_role"].get<int>();

            // (opcional) Validaciones simples
            if (approverEmail.empty() || approverPassword.empty() ||
                targetUserEmail.empty()) {
               res.status = 400;
               res.set_content("Email and password fields cannot be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            bool levelChanged = changeLevelUserUseCase.execute(approverEmail, approverPassword, targetUserEmail, newRole);
            nlohmann::json responseBody;

            // 5. si no se pudo cambiar el nivel
            if (!levelChanged) {
               res.status = 500;
               responseBody["status"] = "error";
               responseBody["message"] = "User level could not be changed";
               res.set_content(responseBody.dump(), "application/json");
               std::cout << "Failed to change level for user with email " << targetUserEmail << std::endl << std::endl;
               return;
            }

            // 6. Construir respuesta JSON si todo salió bien
            responseBody["status"] = "ok";
            responseBody["target_user_email"] = targetUserEmail;
            responseBody["new_role"] = newRole;
            res.status = 200; // OK
            res.set_content(responseBody.dump(), "application/json");
            std::cout << "User level changed for " << targetUserEmail << " to role " << newRole << std::endl << std::endl;

         }
         catch (const nlohmann::json::parse_error &e) {
            // Error al parsear JSON
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            // Error de negocio u otro tipo
            res.status = 500;
            std::cout << "Error changing user level: " << e.what() << std::endl << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );



   /***********************************   VERIFICAR A UN USUARIO NUEVO  ***********************************/
   server_.Post("/user/verify_email",
      [&verifyUserUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            // 1. Verificar que haya body
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }
            
            // 2. Parsear JSON del body
            nlohmann::json body = nlohmann::json::parse(req.body);

            // 3. Extraer campos necesarios
            if (!body.contains("approver_email") || !body.contains("approver_password") || !body.contains("target_user_email")) {
               res.status = 400;
               res.set_content("Missing required fields", "text/plain");
               return;
            }

            std::string approverEmail    = body["approver_email"].get<std::string>();
            std::string approverPassword = body["approver_password"].get<std::string>();
            std::string targetUserEmail  = body["target_user_email"].get<std::string>();

            // (opcional) Validaciones simples
            if (approverEmail.empty() || approverPassword.empty() || targetUserEmail.empty()) {
               res.status = 400;
               res.set_content("Email and password fields cannot be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            bool statusChanged = verifyUserUseCase.execute(approverEmail, approverPassword, targetUserEmail);
            nlohmann::json responseBody;

            // 5. si no se pudo verificar el usuario
            if (!statusChanged) {
               res.status = 500;
               responseBody["status"] = "error";
               responseBody["message"] = "User could not be verified";
               res.set_content(responseBody.dump(), "application/json");
               std::cout << "Failed to change verify for user with email " << targetUserEmail << std::endl << std::endl;
               return;
            }

            // 6. Construir respuesta JSON si todo salió bien
            responseBody["status"] = "ok";
            responseBody["target_user_email"] = targetUserEmail;
            res.status = 200; // OK
            res.set_content(responseBody.dump(), "application/json");
            std::cout << "Verified user email for " << targetUserEmail << std::endl << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            // Error al parsear JSON
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            // Error de negocio u otro tipo
            res.status = 500;
            std::cout << "Error changing user status: " << e.what() << std::endl << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );



   /***********************************   CAMBIO DE STATUS A UN USUARIO  ***********************************/
   server_.Post("/user/change_status",
      [&changeUserStatusUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            // 1. Verificar que haya body
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }

            // 2. Parsear JSON del body
            nlohmann::json body = nlohmann::json::parse(req.body);

            // 3. Extraer campos necesarios
            if (!body.contains("approver_email") || !body.contains("approver_password") || !body.contains("target_user_email") || !body.contains("new_status")) {
               res.status = 400;
               res.set_content("Missing required fields", "text/plain");
               return;
            }

            std::string approverEmail    = body["approver_email"].get<std::string>();
            std::string approverPassword = body["approver_password"].get<std::string>();
            std::string targetUserEmail  = body["target_user_email"].get<std::string>();
            int newStatus                = body["new_status"].get<int>();

            // (opcional) Validaciones simples
            if (approverEmail.empty() || approverPassword.empty() || targetUserEmail.empty()) {
               res.status = 400;
               res.set_content("Email and password fields cannot be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            bool statusChanged = changeUserStatusUseCase.execute(approverEmail, approverPassword, targetUserEmail, newStatus);
            nlohmann::json responseBody;

            // 5. si no se pudo cambiar el status
            if (!statusChanged) {
               res.status = 500;
               responseBody["status"] = "error";
               responseBody["message"] = "User status could not be changed";
               res.set_content(responseBody.dump(), "application/json");
               std::cout << "Failed to change status for user with email " << targetUserEmail << std::endl << std::endl;
               return;
            }

            // 6. Construir respuesta JSON si todo salió bien
            responseBody["status"] = "ok";
            responseBody["target_user_email"] = targetUserEmail;
            responseBody["new_status"] = newStatus;
            res.status = 200; // OK
            res.set_content(responseBody.dump(), "application/json");
            std::cout << "User status changed for " << targetUserEmail << " to status " << newStatus << std::endl << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            // Error al parsear JSON
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            // Error de negocio u otro tipo
            res.status = 500;
            std::cout << "Error changing user status: " << e.what() << std::endl << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );

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