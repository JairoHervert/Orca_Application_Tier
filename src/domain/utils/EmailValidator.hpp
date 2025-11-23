#pragma once
#include <regex>
#include <string>

inline bool isValidEmailFormat(const std::string &email) {
   // Regex practica para validar formato de email
   static const std::regex pattern(
      R"(^[a-z0-9._%+\-]+@[a-z0-9.\-]+\.[a-z]{2,}$)"
   );

   return std::regex_match(email, pattern);
}