#pragma once
#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>
#include "../../domain/repositories/IProjectDB.repository.hpp"

class DBProjectRepository : public IProjectRepositoryDB {
public:
   explicit DBProjectRepository(soci::session &sqlSession)
      : sql_(sqlSession) {}

   std::optional<Repository> findByName(const std::string &name) override {
      soci::row row;
      sql_ << "SELECT idproject, projectname, description, idowner "
               "FROM projects "
               "WHERE projectname = :name "
               "LIMIT 1",
            soci::into(row),
            soci::use(name, "name");

      if (!sql_.got_data()) {
         return std::nullopt;
      }

      Repository p;
      p.idProject   = row.get<int>(0);
      p.name        = row.get<std::string>(1);
      p.description = row.get<std::string>(2);
      p.ownerId     = row.get<int>(3);

      return p;
   }

   Repository create(const std::string &name, const std::string &description, int ownerId) override {
      try {
         int idProject = 0;

         soci::statement st = (sql_.prepare <<
            "INSERT INTO projects (projectname, description, idowner) "
            "VALUES (:name, :description, :ownerId)",
            soci::use(name,        "name"),
            soci::use(description, "description"),
            soci::use(ownerId,     "ownerId")
         );

         st.execute(true);

         // Recuperar id autoincremental
         sql_ << "SELECT LAST_INSERT_ID()", soci::into(idProject);

         Repository p;
         p.idProject   = idProject;
         p.name        = name;
         p.description = description;
         p.ownerId     = ownerId;

         return p;

      } catch (const std::exception &e) {
         // opcional: loggear
         // std::cerr << "[DBProjectRepository::create] " << e.what() << "\n";
         throw; // que suba la excepción al caso de uso
      }
   }

private:
   soci::session &sql_;
};
