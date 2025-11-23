#pragma once
#include <string>

struct User {
   int idUser;
   std::string name;
   std::string email;
   int type;
   std::string publicKeyECDSA;
};