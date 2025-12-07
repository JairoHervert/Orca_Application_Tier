#pragma once
#include <string>
#include <optional>

struct PushOperation {
   std::string op;         // "update" o "delete"
   std::string path;       // ruta relativa, ej: "src/main.cpp"
   std::string signature;  // firma base64
};
