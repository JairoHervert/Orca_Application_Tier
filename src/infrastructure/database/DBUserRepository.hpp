#pragma once
#include <iostream>
#include <string>
#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>
#include "../../domain/repositories/IUserRepository.user.hpp"

class DBUserRepository : public IUserRepository {
public:
   explicit DBUserRepository(soci::session& sqlSession)
      : sql_(sqlSession) {}


   User create(const std::string &name, const std::string &email, const std::string &type) override {
      sql_ << "INSERT INTO Users (Name, Email, Type, Status) "
            "VALUES (:name, :email, :type, 'Active')",
         soci::use(name),
         soci::use(email),
         soci::use(type);

      auto userOpt = findByEmail(email);
      if (!userOpt.has_value()) {
         // Esto no deberia pasar salvo que algo muy raro ocurra
         throw std::runtime_error("Failed to read user after insert");
      }

      User user = userOpt.value();

      return user;
   }



   std::optional<User> findByEmail(const std::string &email) override {
      soci::row row;

      sql_ << "SELECT IdUser, Name, Email, Type, PublicKey FROM Users WHERE Email = :email LIMIT 1",
         soci::into(row),
         soci::use(email);

      if (!sql_.got_data()) {
         // No se encontró ningún usuario con ese email
         std::cout << "No user found with email: " << email << std::endl;
         return std::nullopt;
      }

      User user;
      user.idUser = row.get<int>(0);              // columna 0: idUser
      user.name   = row.get<std::string>(1);      // columna 1: name
      user.email  = row.get<std::string>(2);      // columna 2: email
      user.type   = row.get<int>(3);               // columna 3: type
      user.publicKeyECDSA = row.get<std::string>(4); // columna 4: publicKeyECDSA

      return user;
   }


   bool isDeveloperUser(const std::string &email) override {
      auto userOpt = findByEmail(email);
      return userOpt.has_value() && userOpt->type == 1;
   }
   
   bool isLeaderUser(const std::string &email) override {
      auto userOpt = findByEmail(email);
      return userOpt.has_value() && userOpt->type == 2;
   }

   bool isSeniorUser(const std::string &email) override {
      auto userOpt = findByEmail(email);
      return userOpt.has_value() && userOpt->type == 3;
   }

private:
   soci::session& sql_;
};
