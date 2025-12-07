#pragma once
#include <optional>
#include <string>

class IPushRepoCryptoRepository {
public:
   virtual ~IPushRepoCryptoRepository() = default;

   virtual std::string b64_hash_file_SHA256(const std::string &filePath) = 0;

};