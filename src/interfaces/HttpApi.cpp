#include "HttpApi.hpp"
#include <iostream>

HttpApi::HttpApi(const char* certPath, const char* keyPath)
   : server_(certPath, keyPath) {
   // Constructor vacío o configuración inicial si necesitas
}

void HttpApi::registerRoutes() {
   // Registra todas tus rutas aquí
   server_.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
      res.set_content("Hello World https!", "text/plain");
   });

   server_.Post("/repo/create", [](const httplib::Request&, httplib::Response& res) {
      res.set_content("Repository created!", "text/plain");
   });

   server_.Get("/repo/clone", [](const httplib::Request&, httplib::Response& res) {
      res.set_content("Repository cloned!", "text/plain");
   });
   
   // Aquí agregar las deamas rutas conforme sean necesarias
   // server_.Post("/repo/create", [](const httplib::Request& req, httplib::Response& res) {
   //     // ... lógica
   // });
}

void HttpApi::listen(const char* host, int port) {
   std::cout << "Intentando iniciar servidor HTTPS en https://" << host << ":" << port << std::endl;
   
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