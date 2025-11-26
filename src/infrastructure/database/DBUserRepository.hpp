#pragma once
#include <iostream>
#include <string>
#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>
#include "../../domain/repositories/IUser.repository.hpp"

class DBUserRepository : public IUserRepository {
public:
   explicit DBUserRepository(soci::session& sqlSession)
      : sql_(sqlSession) {}


   bool create(const std::string &name, const std::string &email, const std::string &password) override {
      try {
         soci::statement st = (sql_.prepare <<
            "INSERT INTO users (email, name, password) "
            "VALUES (:email, :name, :password)",
            soci::use(email, "email"),
            soci::use(name, "name"),
            soci::use(password, "password")
         );


         st.execute(true); // true = ejecutar inmediatamente

         std::size_t affected = st.get_affected_rows();
         return affected == 1; // 1 fila insertada

      } catch (const std::exception &e) {
         // registrar el error,,, opacional
         // std::cerr << "Error en create(): " << e.what() << "\n";
         return false;
      }
   }


   bool addPublicKeyECDSA(const std::string &email, const std::string &publicKey) {
      try {
         soci::statement st = (sql_.prepare <<
            "UPDATE users "
            "SET kpubecdsa = :publicKey "
            "WHERE email = :email",
            soci::use(publicKey, "publicKey"),
            soci::use(email, "email")
         );

         st.execute(true);

         std::size_t affected = st.get_affected_rows();
         return affected == 1;

      } catch (const std::exception &e) {
         // std::cerr << "Error en addPublicKeyECDSA(): " << e.what() << "\n";
         return false;
      }
   }

   bool addPublicKeyRSA(const std::string &email, const std::string &publicKey) override {
      try {
         soci::statement st = (sql_.prepare <<
            "UPDATE users "
            "SET kpubrsa = :publicKey "
            "WHERE email = :email",
            soci::use(publicKey, "publicKey"),
            soci::use(email, "email")
         );

         st.execute(true);

         std::size_t affected = st.get_affected_rows();
         return affected == 1;

      } catch (const std::exception &e) {
         // std::cerr << "Error en addPublicKeyRSA(): " << e.what() << "\n";
         return false;
      }
   }



   std::optional<User> findByEmail(const std::string &email) override {
      soci::row row;

      sql_ << "SELECT iduser, name, email, role, status, verify, kpubecdsa, kpubrsa FROM users WHERE email = :email LIMIT 1",
         soci::into(row),
         soci::use(email, "email");

      if (!sql_.got_data()) {
         // No se encontró ningún usuario con ese email
         // std::cout << "No user found with email: " << email << std::endl;
         return std::nullopt;
      }

      User user;
      user.idUser = row.get<int>(0);              // columna 0: idUser
      user.name   = row.get<std::string>(1);      // columna 1: name
      user.email  = row.get<std::string>(2);      // columna 2: email
      user.role   = row.get<int>(3);              // columna 3: role
      user.status = row.get<int>(4);              // columna 4: status
      user.verify = row.get<int>(5);              // columna 5: verify
      
      // --- Manejo de posible NULL en la columna kpubecdsa ---
      soci::indicator indECDSA = row.get_indicator(6);  
      if (indECDSA == soci::i_null) user.publicKeyECDSA = "NULL";
      else user.publicKeyECDSA = row.get<std::string>(6);

      // --- Manejo de posible NULL en la columna kpubrsa ---
      soci::indicator indRSA = row.get_indicator(7);
      if (indRSA == soci::i_null) user.publicKeyRSA = "NULL";
      else user.publicKeyRSA = row.get<std::string>(7);
   
      return user;
   }

   std::optional<User> findById(int idUser) override {
      soci::row row;

      sql_ << "SELECT iduser, name, email, role, status, verify, kpubecdsa, kpubrsa FROM users WHERE iduser = :idUser LIMIT 1",
         soci::into(row),
         soci::use(idUser, "idUser");

      if (!sql_.got_data()) {
         return std::nullopt;
      }

      User user;
      user.idUser = row.get<int>(0);              // columna 0: idUser
      user.name   = row.get<std::string>(1);      // columna 1: name
      user.email  = row.get<std::string>(2);      // columna 2: email
      user.role   = row.get<int>(3);              // columna 3: role
      user.status = row.get<int>(4);              // columna 4: status
      user.verify = row.get<int>(5);              // columna 5: verify
      
      // --- Manejo de posible NULL en la columna kpubecdsa ---
      soci::indicator indECDSA = row.get_indicator(6);  
      if (indECDSA == soci::i_null) user.publicKeyECDSA = "NULL";
      else user.publicKeyECDSA = row.get<std::string>(6);

      // --- Manejo de posible NULL en la columna kpubrsa ---
      soci::indicator indRSA = row.get_indicator(7);
      if (indRSA == soci::i_null) user.publicKeyRSA = "NULL";
      else user.publicKeyRSA = row.get<std::string>(7);
   
      return user;
   }


   bool isValidPassword(const std::string &email, const std::string &password) override {
      int count = 0;
      sql_ << "SELECT COUNT(*) FROM users WHERE email = :email AND password = :password",
         soci::into(count),
         soci::use(email, "email"),
         soci::use(password, "password");

      return count > 0;
   }

   bool isVerifiedUser(const std::string &email) {
      auto userOpt = findByEmail(email);
      return userOpt.has_value() && userOpt->verify == 1;
   }

   bool isStatusActive(const std::string &email) {
      auto userOpt = findByEmail(email);
      return userOpt.has_value() && userOpt->status == 1;
   }

   bool isDeveloperUser(const std::string &email) override {
      auto userOpt = findByEmail(email);
      return userOpt.has_value() && userOpt->role == 1;
   }
   
   bool isLeaderUser(const std::string &email) override {
      auto userOpt = findByEmail(email);
      return userOpt.has_value() && userOpt->role == 2;
   }

   bool isSeniorUser(const std::string &email) override {
      auto userOpt = findByEmail(email);
      return userOpt.has_value() && userOpt->role == 3;
   }

   bool notECDSAKeyAdded(const std::string &email) {
      soci::row row;

      sql_ << "SELECT kpubecdsa FROM users WHERE email = :email LIMIT 1",
         soci::into(row),
         soci::use(email, "email");

      if (!sql_.got_data()) {
         // No se encontró ningún usuario con ese email
         return false;
      }

      soci::indicator ind = row.get_indicator(0);  // columna 0: kpubecdsa

      return ind == soci::i_null;
   }

   bool notRSAKeyAdded(const std::string &email) override {
      soci::row row;

      sql_ << "SELECT kpubrsa FROM users WHERE email = :email LIMIT 1",
         soci::into(row),
         soci::use(email, "email");

      if (!sql_.got_data()) {
         // No se encontró ningún usuario con ese email
         return false;
      }

      soci::indicator ind = row.get_indicator(0);  // columna 0: kpubrsa

      return ind == soci::i_null;
   }


   // Para cambiar el rol de un usuario a entre lider, senior o developer
   bool changeLevelUser(const std::string &email, int newRole) {
      try {
         soci::statement st = (sql_.prepare <<
            "UPDATE users "
            "SET role = :newRole "
            "WHERE email = :email",
            soci::use(newRole, "newRole"),
            soci::use(email, "email")
         );
         st.execute(true);
         std::size_t affected = st.get_affected_rows();
         return affected == 1;
      } catch (const std::exception &e) {
         return false;
      }
   }

   // Para verificar el email de un usuario nuevo
   bool verifyUserEmail(const std::string &email) {
      try {
         soci::statement st = (sql_.prepare <<
            "UPDATE users "
            "SET verify = 1 "
            "WHERE email = :email",
            soci::use(email, "email")
         );
         st.execute(true);
         std::size_t affected = st.get_affected_rows();
         return affected == 1;
      } catch (const std::exception &e) {
         return false;
      }
   }

   bool changeActiveStatus(const std::string &email, int newStatus) {
      try {
         soci::statement st = (sql_.prepare <<
            "UPDATE users "
            "SET status = :newStatus "
            "WHERE email = :email",
            soci::use(newStatus, "newStatus"),
            soci::use(email, "email")
         );
         st.execute(true);
         std::size_t affected = st.get_affected_rows();
         return affected == 1;
      } catch (const std::exception &e) {
         return false;
      }
   }

private:
   soci::session& sql_;
};
