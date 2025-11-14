#include "HttpApi.hpp"
// aquí podrías incluir lib de JSON (nlohmann/json, etc.)

HttpApi::HttpApi() {
   // Si quisieras, puedes configurar middlewares, logs, etc.
}

// void HttpApi::registerRoutes(CreateRepositoryUseCase &createRepoUseCase)
// {
//    server_.Post("/repo/create",
//                 [&createRepoUseCase](const httplib::Request &req,
//                                      httplib::Response &res)
//                 {
//                    // 1. Parsear JSON de req.body (omito detalles)
//                    std::string name = /* leer de JSON */;
//                    std::string owner = /* leer de JSON */;

//                    try
//                    {
//                       // 2. Llamar caso de uso (aplicación)
//                       Repository repo = createRepoUseCase.execute(name, owner);

//                       // 3. Construir respuesta JSON
//                       res.status = 200;
//                       res.set_content("{\"status\":\"ok\"}", "application/json");
//                    }
//                    catch (const std::exception &e)
//                    {
//                       res.status = 400;
//                       res.set_content(
//                           std::string("{\"status\":\"error\",\"message\":\"") + e.what() + "\"}", "application/json");
//                    }
//                 });
// }

void HttpApi::listen(const char *host, int port) {
   server_.listen(host, port);
}
