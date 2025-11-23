#pragma once
#include <optional>
#include <string>
#include "../entities/Repository.entity.hpp"

class IProjectRepositoryDB {
public:
   virtual ~IProjectRepositoryDB() = default;

   virtual std::optional<Repository> findByName(const std::string &name) = 0;

   virtual Repository create(const std::string &name, const std::string &description, int ownerId) = 0;
};
