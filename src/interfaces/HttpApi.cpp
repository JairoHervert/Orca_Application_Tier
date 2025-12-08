#include <iostream>
#include "HttpApi.hpp"
#include "../third_party/json.hpp"
#include "../domain/entities/PushOperation.entity.hpp"

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
   AddUserToRepoUseCase &addUserToRepoUseCase,
   CloneRepositoryUseCase &cloneRepoUseCase,
   DecipherRepositoryUseCase &decipherRepoUseCase,
   HashFilesUseCase &hashRepoFilesCreate,
   PushVerifyUseCase &pushVerifyUseCase,
   AddUserToFileUseCase &addUserToFileUseCase,
   CommitsListUseCase &commitsListUseCase,
   GetAESKeyUseCase &getAESKeyUseCase,
   ListAllProjectsUseCase &listAllProjectsUseCase,
   ListEncryptedProjectsUseCase &listEncryptedProjectsUseCase,
   ListUserAccessibleProjectsUseCase &listUserAccessibleProjectsUseCase,
   ListUserFilesInProjectUseCase &listUserFilesInProjectUseCase,

   TestUseCase &testUseCase  // Caso de uso exclusivo para pruebas
) {
   /***********************************  ENDPOINT PARA PRUEBAS  ***********************************/
   server_.Post("/test",
      [&testUseCase](const httplib::Request& req, httplib::Response& res) {

         nlohmann::json body = nlohmann::json::parse(req.body);
         std::string argument = body["argument"].get<std::string>();

         // Ejecutar el caso de uso de prueba
         std::cout << "Executing TestUseCase with argument: " << argument << std::endl;
         bool hecho = testUseCase.execute(argument);

         std::cout << "TestUseCase execution result: " << (hecho ? "success" : "failure") << std::endl;

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
   server_.Post("/repo/clone", 
      [&cloneRepoUseCase](const httplib::Request& req, httplib::Response& res) {
         try {

            // 1. Verificar que haya body
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }

            // 2. Extraer JSON del body
            nlohmann::json body = nlohmann::json::parse(req.body);

            // 3. Extraer campos "repo_name", "user_email" y "user_password"
            if (!body.contains("repoName") || !body.contains("userEmail") || !body.contains("userPassword")) {
               res.status = 400;
               res.set_content("Missing 'repoName', 'userEmail' or 'userPassword' field", "text/plain");
               return;
            }

            std::string repoName    = body["repoName"].get<std::string>();
            std::string userEmail   = body["userEmail"].get<std::string>();
            std::string userPassword = body["userPassword"].get<std::string>();

            // 4. Validaciones simples
            if (repoName.empty() || userEmail.empty() || userPassword.empty()) {
               res.status = 400;
               res.set_content("Fields 'repoName', 'userEmail' and 'userPassword' cannot be empty", "text/plain");
               return;
            }

            // 5. Ejecutar caso de uso
            std::ostringstream repoStream = cloneRepoUseCase.execute(repoName, userEmail, userPassword);

            // Enviar repositorio como archivo tar adjunto
            std::string fileContent = repoStream.str();
            res.set_content(fileContent, "application/octet-stream");
            res.set_header("Content-Disposition", "attachment; filename=\"" + repoName + "\"");
            res.status = 200;

            std::cout << "Repository cloned: " << repoName << " by user " << userEmail << std::endl << std::endl;
         }
         catch (const std::exception &e) {
            res.status = 500;
            std::cout << "Error cloning repository: " << e.what() << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
         catch (...) {
            res.status = 500;
            std::cout << "Unknown error occurred while cloning repository." << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
         }
      }
   );

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


   /***********************************   AGREGAR UN USUARIO A UN REPOSITORIO  ***********************************/
   server_.Post("/repo/add_user",
      [&addUserToRepoUseCase](const httplib::Request& req, httplib::Response& res) {
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
            if (!body.contains("approverEmail") || !body.contains("approverPassword") || !body.contains("projectName") || !body.contains("userEmail")) {
               res.status = 400;
               res.set_content("Missing required fields", "text/plain");
               return;
            }

            std::string approverEmail    = body["approverEmail"].get<std::string>();
            std::string approverPassword = body["approverPassword"].get<std::string>();
            std::string idProject        = body["projectName"].get<std::string>();
            std::string idUser           = body["userEmail"].get<std::string>();

            // Validacion de campos
            if (approverEmail.empty() || approverPassword.empty() || idProject.empty() || idUser.empty()) {
               res.status = 400;
               res.set_content("Email and password fields cannot be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            addUserToRepoUseCase.execute(approverEmail, approverPassword, idProject, idUser);

            // mandar respuesta al cliente
            nlohmann::json responseBody;
            responseBody["status"] = "ok";
            responseBody["project_name"] = idProject;
            responseBody["user_email"] = idUser;
            res.status = 200; // OK
            res.set_content(responseBody.dump(), "application/json");
            std::cout << "User with ID " << idUser << " added to project with ID " << idProject << std::endl << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            // Error al parsear JSON
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            // Error de negocio u otro tipo
            res.status = 500;
            std::cout << "Error adding user to project: " << e.what() << std::endl << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
         catch (...) {
            // Capturar cualquier otro tipo de excepción
            res.status = 500;
            std::cout << "Unknown error occurred while adding user to project." << std::endl << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
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
   server_.Post("/repo/unprotect", 
      [&decipherRepoUseCase](const httplib::Request& req, httplib::Response& res) {
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
            if (!body.contains("userEmail") || !body.contains("userPassword") || !body.contains("repoName")) {
               res.set_content("Missing required fields", "text/plain");
               return;
            }

            std::string userEmail    = body["userEmail"].get<std::string>();
            std::string userPassword = body["userPassword"].get<std::string>();
            std::string repoName       = body["repoName"].get<std::string>();

            // (opcional) Validaciones simples
            if (userEmail.empty() || userPassword.empty() || repoName.empty()) {
               res.status = 400;
               res.set_content("Email, password, repository cipher name fields cannot be empty", "text/plain");
               return;
            }

            // 4. Ejecutar caso de uso
            std::ostringstream cipherRepoStream = decipherRepoUseCase.execute(repoName, userEmail, userPassword);

            // Enviar repositorio como archivo tar adjunto
            std::string fileContent = cipherRepoStream.str();
            res.set_content(fileContent, "application/octet-stream");
            res.set_header("Content-Disposition", "attachment; filename=\"" + repoName + "\"");
            res.status = 200;

            std::cout << "Repository cipher: " << repoName << " send to user " << userEmail << std::endl << std::endl;
         }
         catch (const std::exception &e) {
            res.status = 500;
            std::cout << "Error sending ciphered repository: " << e.what() << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
         catch (...) {
            res.status = 500;
            std::cout << "Unknown error occurred while sending ciphered repository." << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
         }
      }
   );


   /***********************************   OBTENER HASHES DE ARCHIVOS DE UN REPOSITORIO  ***********************************/
   server_.Post("/repo/push/hash",
      [&hashRepoFilesCreate](const httplib::Request& req, httplib::Response& res) {
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
            if (!body.contains("repoName") || !body.contains("userEmail") || !body.contains("userPassword")) {
               res.status = 400;
               res.set_content("Missing 'repoName', 'userEmail' or 'userPassword' field", "text/plain");
               return;
            }

            std::string repoName = body["repoName"].get<std::string>();
            std::string userEmail = body["userEmail"].get<std::string>();
            std::string userPassword = body["userPassword"].get<std::string>();

            // 4. Validaciones simples
            if (repoName.empty() || userEmail.empty() || userPassword.empty()) {
               res.status = 400;
               res.set_content("Fields cannot be empty", "text/plain");
               return;
            }

            // 5. Ejecutar caso de uso
            std::map<std::string, std::string> fileHashes = hashRepoFilesCreate.execute(repoName, userEmail, userPassword);

            // 6. Construir respuesta JSON
            nlohmann::json responseBody;
            responseBody["status"] = "ok";
            responseBody["repository"] = repoName;
            responseBody["files"] = nlohmann::json::object();

            for (const auto& [path, hash] : fileHashes) {
               responseBody["files"][path] = hash;
            }

            res.status = 200;
            res.set_content(responseBody.dump(2), "application/json"); // dump(2) para formato legible
            
            std::cout << "File hashes generated for repository: " << repoName << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            res.status = 500;
            std::cout << "Error generating file hashes: " << e.what() << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );


   /***********************************   PUSH: RECIBIR ARCHIVOS MODIFICADOS  ***********************************/
   server_.Post("/repo/push/upload",
      [&pushVerifyUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            if (!req.is_multipart_form_data()) {
               res.status = 400;
               res.set_content("Expected multipart/form-data", "text/plain");
               return;
            }

            auto get_text_field = [&](const std::string &name) -> std::string {
               if (req.has_param(name.c_str())) {
                  return req.get_param_value(name.c_str());
               } else if (req.has_file(name.c_str())) {
                  auto f = req.get_file_value(name.c_str());
                  return f.content;
               }
               return "";
            };

            std::string repoName     = get_text_field("repoName");
            std::string userEmail    = get_text_field("userEmail");
            std::string userPassword = get_text_field("userPassword");
            std::string operationsJson = get_text_field("operations"); // 👈 ahora viene un JSON de operaciones

            if (repoName.empty() || userEmail.empty() || userPassword.empty() || operationsJson.empty()) {
               res.status = 400;
               res.set_content("Missing required fields (repoName, userEmail, userPassword, operations)", "text/plain");
               return;
            }

            // 1) Parsear JSON de operaciones -> vector<PushOperation>
            nlohmann::json opsJson = nlohmann::json::parse(operationsJson);

            std::vector<PushOperation> operations;

            // Soportar tanto:
            //  - { "operations": [ {...}, {...} ] }
            //  - [ {...}, {...} ]
            nlohmann::json opsArray;
            if (opsJson.is_array()) {
               opsArray = opsJson;
            } else if (opsJson.contains("operations") && opsJson["operations"].is_array()) {
               opsArray = opsJson["operations"];
            } else {
               res.status = 400;
               res.set_content("Invalid 'operations' JSON format", "text/plain");
               return;
            }

            for (const auto &item : opsArray) {
               if (!item.contains("op") || !item.contains("path") || !item.contains("signature")) {
                  res.status = 400;
                  res.set_content("Each operation must contain 'op', 'path' and 'signature'", "text/plain");
                  return;
               }

               PushOperation op;
               op.op        = item["op"].get<std::string>();        // "update" o "delete"
               op.path      = item["path"].get<std::string>();
               op.signature = item["signature"].get<std::string>();

               if (op.op != "update" && op.op != "delete") {
                  res.status = 400;
                  res.set_content("Invalid operation type: " + op.op, "text/plain");
                  return;
               }

               operations.push_back(op);
            }

            // 2) Extraer archivo tar (opcional: solo necesario si hay operaciones 'update')
            std::string tarFilename;
            std::string tarContent;

            if (req.has_file("tarFile")) {
               auto tarFile = req.get_file_value("tarFile");
               tarFilename  = tarFile.filename;           // p.ej. "cambios.tar"
               tarContent   = std::move(tarFile.content); // binario

               std::cout << "✓ Archivo .tar recibido en memoria: "
                        << tarFilename << " (" << tarContent.size() << " bytes)" << std::endl;
            } else {
               // Si no hay tarFile, solo se podrán procesar operaciones 'delete'
               std::cout << "No tarFile provided; only DELETE operations will be applicable." << std::endl;
            }

            // 3) Llamar al caso de uso: él se encargará de guardar el tar (si lo hay), descomprimir, verificar, etc.
            bool ok = pushVerifyUseCase.execute(
               userEmail,
               userPassword,
               repoName,
               tarFilename,
               tarContent,
               operations
            );

            // 4) Respuesta al cliente
            std::size_t updates = 0;
            std::size_t deletes = 0;
            for (const auto &op : operations) {
               if (op.op == "update") ++updates;
               else if (op.op == "delete") ++deletes;
            }

            nlohmann::json responseBody;
            responseBody["repoName"]        = repoName;
            responseBody["operationsTotal"] = operations.size();
            responseBody["updates"]         = updates;
            responseBody["deletes"]         = deletes;
            responseBody["tarSize"]         = tarContent.size();

            if (ok) {
               responseBody["status"]  = "ok";
               responseBody["message"] = "Push verified and applied successfully";
               res.status = 200;
            } else {
               responseBody["status"]  = "rejected";
               responseBody["message"] = "Push rejected due to invalid signatures or permissions";
               res.status = 400;
            }

            res.set_content(responseBody.dump(2), "application/json");

         } catch (const nlohmann::json::parse_error &e) {
            res.status = 400;
            res.set_content(std::string("Invalid JSON in 'operations': ") + e.what(), "text/plain");
         } catch (const std::exception &e) {
            res.status = 500;
            std::cerr << "Error processing push: " << e.what() << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );


   /***********************************   AGREGAR UN USUARIO A UN ARCHIVO  ***********************************/
   server_.Post("/repo/file/add_user",
      [&addUserToFileUseCase](const httplib::Request& req, httplib::Response& res) {
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
            if (!body.contains("approver_email") || !body.contains("approver_password")||
               !body.contains("file_name") || !body.contains("project_name") || !body.contains("user_email")) {
               res.status = 400;
               res.set_content(
                  "Missing required fields: "
                  "'approver_email', 'approver_password', "
                  "'file_name', 'project_name', 'user_email'",
                  "text/plain"
               );
               return;
            }

            std::string approverEmail    = body["approver_email"].get<std::string>();
            std::string approverPassword = body["approver_password"].get<std::string>();
            std::string fileName         = body["file_name"].get<std::string>();
            std::string projectName      = body["project_name"].get<std::string>();
            std::string userEmail        = body["user_email"].get<std::string>();

            // 4. Validaciones simples de campos vacíos
            if (approverEmail.empty()    || approverPassword.empty() ||
               fileName.empty()         || projectName.empty()      ||
               userEmail.empty()) {
               
               res.status = 400;
               res.set_content("Fields cannot be empty", "text/plain");
               return;
            }

            // 5. Ejecutar caso de uso
            bool ok = addUserToFileUseCase.execute(
               approverEmail,
               approverPassword,
               fileName,
               projectName,
               userEmail
            );

            // 6. Construir respuesta JSON
            nlohmann::json responseBody;
            responseBody["status"]          = ok ? "ok" : "failed";
            responseBody["project_name"]    = projectName;
            responseBody["file_name"]       = fileName;
            responseBody["target_user"]     = userEmail;
            responseBody["approver_email"]  = approverEmail;

            res.status = ok ? 200 : 500;
            res.set_content(responseBody.dump(2), "application/json");

            std::cout << "AddUserToFileUseCase: user " << userEmail
                     << " added to file " << fileName
                     << " in project " << projectName
                     << " by " << approverEmail << std::endl << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         } catch (const std::exception &e) {
            res.status = 500;
            std::cerr << "Error processing push: " << e.what() << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );



   /***********************************   Consultar la lista de commits  ***********************************/
   server_.Get("/repo/commits",
      [&commitsListUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            std::vector<Commit> commits = commitsListUseCase.execute();

            nlohmann::json responseBody;
            responseBody["status"]  = "ok";
            responseBody["commits"] = nlohmann::json::array();

            for (const auto& commit : commits) {
               nlohmann::json commitJson;
               commitJson["id"]          = commit.idcommits;
               commitJson["author"]      = commit.iduser;
               commitJson["file_id"]     = commit.idsourcefile;       // o null si no hay
               commitJson["signature"]   = commit.digitalsignature;   // o null si no hay
               commitJson["accepted"]    = commit.isaccepted;
               commitJson["date"]        = commit.date;
               commitJson["command"]     = commit.command;
               commitJson["description"] = commit.description;

               responseBody["commits"].push_back(commitJson);
            }

            res.status = 200;
            res.set_content(responseBody.dump(2), "application/json");

            std::cout << "Listed " << commits.size() << " commits successfully." << std::endl;
         }
         catch (const std::exception &e) {
            res.status = 500;
            std::cout << "Error listing commits: " << e.what() << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );


   /***********************************   OBTENER AES (RSA-AES) DE UN REPOSITORIO  ***********************************/
   server_.Post("/repo/protect/get_key",
      [&getAESKeyUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }

            nlohmann::json body = nlohmann::json::parse(req.body);

            if (!body.contains("user_email")     ||
               !body.contains("user_password")  ||
               !body.contains("project_name")   ||
               !body.contains("project_alias")) {
               res.status = 400;
               res.set_content(
                  "Missing required fields: 'user_email', 'user_password', "
                  "'project_name', 'project_alias'",
                  "text/plain"
               );
               return;
            }

            std::string userEmail    = body["user_email"].get<std::string>();
            std::string userPassword = body["user_password"].get<std::string>();
            std::string projectName  = body["project_name"].get<std::string>();
            std::string projectAlias = body["project_alias"].get<std::string>();

            if (userEmail.empty() || userPassword.empty() ||
               projectName.empty() || projectAlias.empty()) {
               res.status = 400;
               res.set_content("Fields cannot be empty", "text/plain");
               return;
            }

            // Ejecutar caso de uso
            std::string aesKeyEnc = getAESKeyUseCase.execute(
               userEmail,
               userPassword,
               projectName,
               projectAlias
            );

            nlohmann::json responseBody;
            responseBody["status"]        = "ok";
            responseBody["user_email"]    = userEmail;
            responseBody["project_name"]  = projectName;
            responseBody["project_alias"] = projectAlias;
            responseBody["aes_rsa_key"]   = aesKeyEnc; // clave AES cifrada con RSA

            res.status = 200;
            res.set_content(responseBody.dump(2), "application/json");

            std::cout << "AES key (RSA-wrapped) retrieved for user " << userEmail
                     << " and project " << projectName
                     << " (alias: " << projectAlias << ")"
                     << std::endl << std::endl;
         }
         catch (const nlohmann::json::parse_error &e) {
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         }
         catch (const std::exception &e) {
            res.status = 500;
            std::cout << "Error getting AES key: " << e.what() << std::endl << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
         catch (...) {
            res.status = 500;
            std::cout << "Unknown error occurred while getting AES key." << std::endl << std::endl;
            res.set_content("Internal error: Unknown error occurred", "text/plain");
         }
      }
   );

   /***********************************   LISTAR TODOS LOS REPOSITORIOS  ***********************************/
   server_.Get("/repo/list_all",
      [&listAllProjectsUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            // 1. Ejecutar caso de uso (sin parámetros)
            std::vector<Repository> projects = listAllProjectsUseCase.execute();

            // 2. Construir respuesta JSON
            nlohmann::json responseBody;
            responseBody["status"]   = "ok";
            responseBody["total"]    = projects.size();
            responseBody["projects"] = nlohmann::json::array();

            for (const auto &p : projects) {
               nlohmann::json pj;
               pj["idproject"]   = p.idProject;
               pj["name"]        = p.name;
               pj["description"] = p.description;
               pj["ownerId"]     = p.ownerId;
               responseBody["projects"].push_back(pj);
            }

            res.status = 200;
            res.set_content(responseBody.dump(2), "application/json");

            std::cout << "Listed " << projects.size()
                     << " projects (no auth)" << std::endl << std::endl;

         } catch (const std::exception &e) {
            res.status = 500;
            std::cout << "Error listing all projects: " << e.what() << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(),
                           "text/plain");
         }
      }
   );

   /***********************************   LISTAR REPOSITORIOS CIFRADOS DE UN USUARIO  ***********************************/
   server_.Post("/repo/list_encrypted",
      [&listEncryptedProjectsUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }

            nlohmann::json body = nlohmann::json::parse(req.body);

            if (!body.contains("userEmail") || !body.contains("userPassword")) {
               res.status = 400;
               res.set_content("Missing 'userEmail' or 'userPassword' field", "text/plain");
               return;
            }

            std::string userEmail    = body["userEmail"].get<std::string>();
            std::string userPassword = body["userPassword"].get<std::string>();

            if (userEmail.empty() || userPassword.empty()) {
               res.status = 400;
               res.set_content("Fields 'userEmail' and 'userPassword' cannot be empty", "text/plain");
               return;
            }

            // 1. Ejecutar caso de uso
            std::vector<Repository> projects =
               listEncryptedProjectsUseCase.execute(userEmail, userPassword);

            // 2. Construir respuesta
            nlohmann::json responseBody;
            responseBody["status"]    = "ok";
            responseBody["total"]     = projects.size();
            responseBody["projects"]  = nlohmann::json::array();

            for (const auto &p : projects) {
               nlohmann::json pj;
               pj["idproject"]   = p.idProject;
               pj["name"]        = p.name;
               pj["description"] = p.description;
               pj["ownerId"]     = p.ownerId;
               responseBody["projects"].push_back(pj);
            }

            res.status = 200;
            res.set_content(responseBody.dump(2), "application/json");

            std::cout << "Listed " << projects.size()
                     << " encrypted projects for user " << userEmail
                     << std::endl << std::endl;

         } catch (const nlohmann::json::parse_error &e) {
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         } catch (const std::runtime_error &e) {
            // Aquí puedes separar errores de autorización
            std::string msg = e.what();
            if (msg.find("not allowed to list encrypted") != std::string::npos) {
               res.status = 403; // Forbidden
            } else {
               res.status = 400;
            }
            res.set_content(msg, "text/plain");
         } catch (const std::exception &e) {
            res.status = 500;
            std::cout << "Error listing encrypted projects: " << e.what() << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );


   /***********************************   LISTAR REPOSITORIOS ACCESIBLES DE UN USUARIO  ***********************************/
   server_.Post("/repo/list_accessible",
      [&listUserAccessibleProjectsUseCase](const httplib::Request& req,
                                          httplib::Response& res) {
         try {
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }

            nlohmann::json body = nlohmann::json::parse(req.body);

            if (!body.contains("userEmail") || !body.contains("userPassword")) {
               res.status = 400;
               res.set_content("Missing 'userEmail' or 'userPassword' field", "text/plain");
               return;
            }

            std::string userEmail    = body["userEmail"].get<std::string>();
            std::string userPassword = body["userPassword"].get<std::string>();

            if (userEmail.empty() || userPassword.empty()) {
               res.status = 400;
               res.set_content("Fields 'userEmail' and 'userPassword' cannot be empty", "text/plain");
               return;
            }

            // 1. Ejecutar caso de uso
            std::vector<Repository> projects =
               listUserAccessibleProjectsUseCase.execute(userEmail, userPassword);

            // 2. Construir respuesta
            nlohmann::json responseBody;
            responseBody["status"]   = "ok";
            responseBody["total"]    = projects.size();
            responseBody["projects"] = nlohmann::json::array();

            for (const auto &p : projects) {
               nlohmann::json pj;
               pj["idproject"]   = p.idProject;
               pj["name"]        = p.name;
               pj["description"] = p.description;
               pj["ownerId"]     = p.ownerId;
               responseBody["projects"].push_back(pj);
            }

            res.status = 200;
            res.set_content(responseBody.dump(2), "application/json");

            std::cout << "Listed " << projects.size()
                     << " accessible projects for user " << userEmail
                     << std::endl << std::endl;

         } catch (const nlohmann::json::parse_error &e) {
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         } catch (const std::exception &e) {
            res.status = 500;
            std::cout << "Error listing accessible projects: " << e.what() << std::endl;
            res.set_content(std::string("Internal error: ") + e.what(), "text/plain");
         }
      }
   );


   /***********************************   LISTAR ARCHIVOS DE UN REPOSITORIO PARA UN USUARIO  ***********************************/
   server_.Post("/repo/list_files",
      [&listUserFilesInProjectUseCase](const httplib::Request& req, httplib::Response& res) {
         try {
            if (req.body.empty()) {
               res.status = 400;
               res.set_content("Request body is empty", "text/plain");
               return;
            }

            nlohmann::json body = nlohmann::json::parse(req.body);

            if (!body.contains("userEmail") ||
               !body.contains("userPassword") ||
               !body.contains("projectName")) {
               res.status = 400;
               res.set_content(
                  "Missing 'userEmail', 'userPassword' or 'projectName' field",
                  "text/plain"
               );
               return;
            }

            std::string userEmail    = body["userEmail"].get<std::string>();
            std::string userPassword = body["userPassword"].get<std::string>();
            std::string projectName  = body["projectName"].get<std::string>();

            if (userEmail.empty() || userPassword.empty() || projectName.empty()) {
               res.status = 400;
               res.set_content("Fields cannot be empty", "text/plain");
               return;
            }

            auto files = listUserFilesInProjectUseCase.execute(
               userEmail,
               userPassword,
               projectName
            );

            nlohmann::json responseBody;
            responseBody["status"]  = "ok";
            responseBody["project"] = projectName;
            responseBody["total"]   = files.size();
            responseBody["files"]   = nlohmann::json::array();

            for (const auto &f : files) {
               nlohmann::json fj;
               fj["idsourcefile"] = f.idsourcefile;
               fj["route"]        = f.route;
               responseBody["files"].push_back(fj);
            }

            res.status = 200;
            res.set_content(responseBody.dump(2), "application/json");

         } catch (const nlohmann::json::parse_error &e) {
            res.status = 400;
            res.set_content(std::string("Invalid JSON: ") + e.what(), "text/plain");
         } catch (const std::exception &e) {
            res.status = 500;
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