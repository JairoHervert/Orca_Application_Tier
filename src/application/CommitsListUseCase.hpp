#pragma once
#include <string>
#include <stdexcept>
#include <vector>
#include "../domain/repositories/IProjectDB.repository.hpp"

class CommitsListUseCase
{
public:
   explicit CommitsListUseCase(IProjectRepositoryDB &projectRepositoryDB)
      : projectRepositoryDB_(projectRepositoryDB) {}

   std::vector<Commit> execute() {
      return projectRepositoryDB_.getCommits();
   }

private:
   IProjectRepositoryDB &projectRepositoryDB_;
};
