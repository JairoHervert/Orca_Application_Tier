#pragma once
#include <string>

struct Repository {
   int idProject;
   std::string name;
   std::string description;
   int ownerId;   // FK a users.iduser
};
