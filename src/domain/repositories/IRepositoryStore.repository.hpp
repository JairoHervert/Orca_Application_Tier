#pragma once
#include <optional>
#include <string>
#include <filesystem>
#include "../entities/Repository.entity.hpp"

class IRepositoryStore {
public:
   virtual ~IRepositoryStore() = default;

   virtual std::optional<Repository> findByName(const std::string &name) = 0;

   virtual Repository create(const std::string &name) = 0;

   virtual bool deleteRepositoryFolder(const std::string &name) = 0;

   virtual bool deleteRepositoryFile(const std::string &name) = 0;

   virtual bool deleteCipherFile(const std::string &name) = 0;

   virtual std::filesystem::path folderToTar(const std::string &name) = 0;

   virtual std::filesystem::path tarToFolder(const std::string &name) = 0;

};
