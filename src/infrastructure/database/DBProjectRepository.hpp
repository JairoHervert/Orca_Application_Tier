#pragma once
#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>
#include "../../domain/repositories/IProjectDB.repository.hpp"

class DBProjectRepository : public IProjectRepositoryDB {
public:
   explicit DBProjectRepository(soci::session &sqlSession)
      : sql_(sqlSession) {}


   std::optional<Repository> findById(int idProject) override {
      soci::row row;
      sql_ << "SELECT idproject, projectname, description, idowner "
               "FROM projects "
               "WHERE idproject = :idProject "
               "LIMIT 1",
            soci::into(row),
            soci::use(idProject, "idProject");

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
         std::cerr << "[DBProjectRepository::create] " << e.what() << "\n";
         throw; // que suba la excepción al caso de uso
      }
   }

   bool deleteRepositoryById(int idProject) override {
      try {
         soci::statement st = (sql_.prepare <<
            "DELETE FROM projects WHERE idproject = :idProject",
            soci::use(idProject, "idProject")
         );

         st.execute(true);
         std::size_t affected = st.get_affected_rows();
         return affected == 1;

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::deleteRepositoryById] " << e.what() << "\n";
         return false;
      }
   }

   bool deleteRepositoryByName(const std::string &name) override {
      try {
         soci::statement st = (sql_.prepare <<
            "DELETE FROM projects WHERE projectname = :name",
            soci::use(name, "name")
         );

         st.execute(true);
         std::size_t affected = st.get_affected_rows();
         return affected == 1;

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::deleteRepositoryByName] " << e.what() << "\n";
         return false;
      }
   }

   
   bool existsUserInProject(int idProject, int idUser) {
      try {
         int count = 0;
         sql_ << "SELECT COUNT(*) FROM users_has_projects WHERE idproject = :idProject AND iduser = :idUser",
            soci::into(count),
            soci::use(idProject, "idProject"),
            soci::use(idUser,    "idUser");

         return count > 0;

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::existsUserInProject] " << e.what() << "\n";
         return false;
      }
   }

   bool addUserToProject(int idProject, int idUser) override {
      try {
         soci::statement st = (sql_.prepare <<
            "INSERT INTO users_has_projects (iduser, idproject) "
            "VALUES (:idUser, :idProject)",
            soci::use(idUser,    "idUser"),
            soci::use(idProject, "idProject")
         );
         st.execute(true);
         std::size_t affected = st.get_affected_rows();
         return affected == 1;

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::addUserToProject] " << e.what() << "\n";
         return false;
      }
   }


   bool addPassword_repo_user(int idUser, int idproject, std::string password, std::string projectAlias) override {
      try {
         soci::statement st = (sql_.prepare <<
            "INSERT INTO repo_protect (iduser, idproject, rsa_aes, project_alias) "
            "VALUES (:idUser, :idproject, :password, :projectAlias)",
            soci::use(idUser,   "idUser"),
            soci::use(idproject,"idproject"),
            soci::use(password, "password"),
            soci::use(projectAlias, "projectAlias")
         );
         st.execute(true);
         std::size_t affected = st.get_affected_rows();
         return affected == 1;

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::addPassword_repo_user] " << e.what() << "\n";
         return false;
      }
   }

   bool existsRepoAlias(const std::string &projectAlias) override {
      try {
         int count = 0;
         sql_ << "SELECT COUNT(*) FROM repo_protect WHERE project_alias = :projectAlias",
            soci::into(count),
            soci::use(projectAlias, "projectAlias");

         return count > 0;

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::existsRepoAlias] " << e.what() << "\n";
         return false;
      }
   }

   bool existsUserInCipherProject(const std::string &projectAlias, int idUser) override {
      try {
         int count = 0;
         sql_ << "SELECT COUNT(*) FROM repo_protect WHERE project_alias = :projectAlias AND iduser = :idUser",
            soci::into(count),
            soci::use(projectAlias, "projectAlias"),
            soci::use(idUser, "idUser");

         return count > 0;

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::existsUserInCipherProject] " << e.what() << "\n";
         return false;
      }
   }


   /************* Tabla sourcefiles y filepermissions *************/
   std::optional<SourceFileDB> existsFileInProject(const std::string &relativePath, int idProject) {
      try {
         soci::rowset<soci::row> rs = (
            sql_.prepare
               << "SELECT idsourcefile, idproject, route "
                  "FROM sourcefiles "
                  "WHERE route = :relativePath AND idproject = :idProject "
                  "LIMIT 1",
               soci::use(relativePath, "relativePath"),
               soci::use(idProject,    "idProject")
         );

         auto it = rs.begin();
         if (it == rs.end()) {
            // No hay filas → no existe el archivo en ese proyecto
            return std::nullopt;
         }

         const soci::row &row = *it;

         SourceFileDB file;
         file.idsourcefile = row.get<int>(0);              // idsourcefile
         file.idproject    = row.get<int>(1);              // idproject
         file.route        = row.get<std::string>(2);      // route

         return file;   // std::optional<SopurceFileDB> con valor

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::existsFileInProject] " << e.what() << "\n";
         return std::nullopt;
      }
   }

   bool addFileToProject(const std::string &relativePath, int idProject) override {
      try {
         soci::statement st = (sql_.prepare <<
            "INSERT INTO sourcefiles (route, idproject) "
            "VALUES (:relativePath, :idProject)",
            soci::use(relativePath, "relativePath"),
            soci::use(idProject,    "idProject")
         );
         st.execute(true);
         std::size_t affected = st.get_affected_rows();
         return affected == 1;

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::addFileToProject] " << e.what() << "\n";
         return false;
      }
   }


   bool existsUserFilePermission(int idUser, int idFile) override {
      try {
         int count = 0;
         sql_ << "SELECT COUNT(*) FROM filepermissions WHERE iduser = :idUser AND idsourcefile = :idsourcefile",
            soci::into(count),
            soci::use(idUser, "idUser"),
            soci::use(idFile, "idsourcefile");

         return count > 0;

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::existsUserFilePermission] " << e.what() << "\n";
         return false;
      }
   }

   bool addUserFilePermission(int idUser, int idFile) override {
      try {
         soci::statement st = (sql_.prepare <<
            "INSERT INTO filepermissions (iduser, idsourcefile) "
            "VALUES (:idUser, :idsourcefile)",
            soci::use(idUser, "idUser"),
            soci::use(idFile, "idsourcefile")
         );
         st.execute(true);
         std::size_t affected = st.get_affected_rows();
         return affected == 1;

      } catch (const std::exception &e) {
         std::cerr << "[DBProjectRepository::addUserFilePermission] " << e.what() << "\n";
         return false;
      }
   }



private:
   soci::session &sql_;
};
