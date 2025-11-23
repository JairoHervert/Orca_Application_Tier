#pragma once
#include <string>

struct User {
   int idUser;
   std::string name;
   std::string email;
   int role;      // 1: Developer, 2: Leader, 3: Senior
   int status;    // 0: Inactive, 1: Active
   int verify;    // 0: Not Verified, 1: Verified
   std::string publicKeyECDSA;
};
