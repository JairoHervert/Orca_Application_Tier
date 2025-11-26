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
   ChangeStatusUserUseCase& changeUserStatusUseCase,
   SavePublicKeyRSAUseCase& saveKPubRSAUseCase,
   CipherRepositoryUseCase &cipherRepoUseCase,

   TestUseCase &testUseCase  // Caso de uso exclusivo para pruebas
) {
   /***********************************  ENDPOINT PARA PRUEBAS  ***********************************/
   server_.Post("/test",
      [&testUseCase](const httplib::Request& req, httplib::Response& res) {

         nlohmann::json body = nlohmann::json::parse(req.body);
         std::string argument = body["argument"].get<std::string>();

         // Ejecutar el caso de uso de prueba
         bool hecho = testUseCase.execute(argument);

         if (hecho) {
            res.status = 200; // OK
            res.set_content("Test use case executed successfully.", "text/plain");
         } else {
            res.status = 500; // Internal Server Error
            res.set_content("Test use case failed.", "text/plain");
         }
      }
   );


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
         catch (...) {
            // Capturar cualquier otro tipo de excepción
            res.status = 500;
            std::cout << "Unknown error occurred while creating repository." << std::endl << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
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
            
            // 5. Construir respuesta JSON
            nlohmann::json responseBody;
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
               res.set_content("Missing 'email', 'password' or 'public_key_ecdsa' field", "text/plain");
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
         catch (...) {
            // Capturar cualquier otro tipo de excepción
            res.status = 500;
            std::cout << "Unknown error occurred while saving public key." << std::endl << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
         }
      }
   );


   /***********************************   INSERTAR K_PUB RSA A UN USUARIO  ***********************************/
   server_.Post("/user/add_kpub_rsa",
      [&saveKPubRSAUseCase](const httplib::Request& req, httplib::Response& res) {
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
            if (!body.contains("email") || !body.contains("password") || !body.contains("kpub_rsa")) {
               res.status = 400;
               res.set_content("Missing 'email', 'password' or 'public_key_rsa' field", "text/plain");
               return;
            }

            std::string email    = body["email"].get<std::string>();
            std::string password = body["password"].get<std::string>();
            std::string publicKey = body["kpub_rsa"].get<std::string>();

            // (opcional) Validaciones simples
            if (email.empty() || password.empty() || publicKey.empty()) {
               res.status = 400;
               res.set_content("Fields 'name', 'email' and 'password' cannot be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            bool keySaved = saveKPubRSAUseCase.execute(email, publicKey, password);

            // 5. Construir respuesta JSON
            nlohmann::json responseBody;
            responseBody["status"] = "ok";
            responseBody["user_email"] = email;
            responseBody["key_saved"]  = keySaved;

            res.status = 201; // Created
            res.set_content(responseBody.dump(), "application/json");
            std::cout << "RSA Public key saved for user with email " << email << std::endl << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            // Error al parsear JSON
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            // Error de negocio u otro tipo
            res.status = 500;
            std::cout << "Error saving RSA public key: " << e.what() << std::endl << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
         catch (...) {
            // Capturar cualquier otro tipo de excepción
            res.status = 500;
            std::cout << "Unknown error occurred while saving RSA public key." << std::endl << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
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

            // mandar respuesta al cliente
            nlohmann::json responseBody;
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
         catch (...) {
            // Capturar cualquier otro tipo de excepción
            res.status = 500;
            std::cout << "Unknown error occurred while changing user level." << std::endl << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
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

            // mandar respuesta al cliente
            nlohmann::json responseBody;
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
         catch (...) {
            // Capturar cualquier otro tipo de excepción
            res.status = 500;
            std::cout << "Unknown error occurred while verifying user." << std::endl << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
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

            // mandar respuesta al cliente
            nlohmann::json responseBody;
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



   /***********************************   CIFRAR UN REPOSITORIO  ***********************************/
   server_.Post("/repo/protect",
      [&cipherRepoUseCase](const httplib::Request& req, httplib::Response& res) {
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
            if (!body.contains("leader_email") || !body.contains("leader_password") || !body.contains("senior_email") || !body.contains("repo_name") || !body.contains("repo_tag")) {
               res.status = 400;
               res.set_content("Missing required fields", "text/plain");
               return;
            }

            std::string leaderEmail    = body["leader_email"].get<std::string>();
            std::string leaderPassword = body["leader_password"].get<std::string>();
            std::string seniorEmail    = body["senior_email"].get<std::string>();
            std::string repoName       = body["repo_name"].get<std::string>();
            std::string repo_tag   = body["repo_tag"].get<std::string>();

            // (opcional) Validaciones simples
            if (leaderEmail.empty() || leaderPassword.empty() || seniorEmail.empty() || repoName.empty() || repo_tag.empty()) {
               res.status = 400;
               res.set_content("Email, password and repository name fields cannot be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            std::string aes_rsa_key = cipherRepoUseCase.execute(leaderEmail, leaderPassword, seniorEmail, repoName, repo_tag);

            // mandar respuesta al cliente
            nlohmann::json responseBody;
            responseBody["status"] = "ok";
            responseBody["repo_name"] = repoName;
            responseBody["aes_rsa_key"] = aes_rsa_key;
            responseBody["message"] = "Store this AES key encrypted with RSA safely to decrypt the repository later. You can also retrieve it from the database when needed.";
            res.status = 200; // OK
            res.set_content(responseBody.dump(), "application/json");
            std::cout << "Repository ciphered: " << repoName << " with alias " << repoName + "_" + repo_tag << std::endl << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            // Error al parsear JSON
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            // Error de negocio u otro tipo
            res.status = 500;
            std::cout << "Error ciphering repository: " << e.what() << std::endl << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
         catch (...) {
            // Capturar cualquier otro tipo de excepción
            res.status = 500;
            std::cout << "Unknown error occurred while ciphering repository." << std::endl << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
         }
      }
   );



   /***********************************   DESCIFRAR UN REPOSITORIO  ***********************************/
   
   // chance este pase a ser un get con query params porque le enviaremos el tar cifrado
   server_.Post("/repo/dec_local_protect", [](const httplib::Request&, httplib::Response& res) {
      res.set_content("Repository deciphered!", "text/plain");
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