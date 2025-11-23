#pragma once
#include <optional>
#include <string>
#include "../entities/User.entity.hpp"

class IUserRepository {
public:
   virtual ~IUserRepository() = default;

   virtual User create(const std::string &name, const std::string &email, const std::string &type) = 0;

   virtual std::optional<User> findByEmail(const std::string &email) = 0;

   virtual bool isLeaderUser(const std::string &email) = 0;

   virtual bool isSeniorUser(const std::string &email) = 0;

   virtual bool isDeveloperUser(const std::string &email) = 0;
};
