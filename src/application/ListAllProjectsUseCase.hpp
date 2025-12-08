#pragma once
#include <vector>
#include "../domain/repositories/IProjectDB.repository.hpp"
#include "../domain/entities/Repository.entity.hpp"

class ListAllProjectsUseCase {
public:
   explicit ListAllProjectsUseCase(IProjectRepositoryDB &projectRepositoryDB)
      : projectRepositoryDB_(projectRepositoryDB) {}

   std::vector<Repository> execute() {
      // Delegas en un método del repositorio que devuelva todos los proyectos
      return projectRepositoryDB_.getAllProjects();
   }

private:
   IProjectRepositoryDB &projectRepositoryDB_;
};
