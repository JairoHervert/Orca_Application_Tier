#pragma once
#include <string>
#include <stdexcept>


// el repositorio de interes a probar
#include "../domain/repositories/IRepositoryStore.repository.hpp"


class TestUseCase {
public:
   explicit TestUseCase(IRepositoryStore &repositoryStore)
      : repositoryStore_(repositoryStore) {}


   // modificar los parametros necesarios
   bool execute(const std::string &repoName) {
      

      // deleteRepositoryFolder
      bool deletedFolder = repositoryStore_.deleteCipherFile(repoName);
      if (!deletedFolder) {
         throw std::runtime_error("Failed to delete repository file during test.");
      }

      return true;
   }



private:
   IRepositoryStore  &repositoryStore_;
};