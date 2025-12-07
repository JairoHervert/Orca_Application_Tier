#pragma once
#include <string>
#include <optional>

struct Commit {
   int idcommits;
   int iduser;
   std::optional<int> idsourcefile;           // puede ser nulo
   std::optional<std::string> digitalsignature; // puede ser nulo
   bool isaccepted;
   std::string date;
   std::string command;
   std::optional<std::string> description;    // puede ser nulo
};
