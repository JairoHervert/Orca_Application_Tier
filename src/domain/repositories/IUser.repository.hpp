#pragma once
#include <optional>
#include <string>
#include "../entities/User.entity.hpp"

class IUserRepository {
public:
   virtual ~IUserRepository() = default;

   virtual bool create(const std::string &name, const std::string &email, const std::string &password) = 0;

   virtual bool addPublicKeyECDSA(const std::string &email, const std::string &publicKey) = 0;

   virtual bool addPublicKeyRSA(const std::string &email, const std::string &publicKey) = 0;

   virtual std::optional<User> findByEmail(const std::string &email) = 0;

   virtual bool isValidPassword(const std::string &email, const std::string &password) = 0;

   virtual bool isVerifiedUser(const std::string &email) = 0;

   virtual bool isStatusActive(const std::string &email) = 0;

   virtual bool isDeveloperUser(const std::string &email) = 0;

   virtual bool isLeaderUser(const std::string &email) = 0;

   virtual bool isSeniorUser(const std::string &email) = 0;

   virtual bool notECDSAKeyAdded(const std::string &email) = 0;

   virtual bool notRSAKeyAdded(const std::string &email) = 0;

   virtual bool changeLevelUser(const std::string &email, int newRole) = 0;

   virtual bool verifyUserEmail(const std::string &email) = 0;

   virtual bool changeActiveStatus(const std::string &email, int newStatus) = 0;
};
