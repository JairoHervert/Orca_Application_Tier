#pragma once
#include <iostream>
#include <string>
#include <stdexcept>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/sha.h>
#include <cryptopp/base64.h>
#include "../../domain/repositories/IPushRepoVerifyCrypto.repository.hpp"

class PushRepoVerifyCrypto : public IPushRepoCryptoRepository {
public:
   explicit PushRepoVerifyCrypto() = default;

   // Calcular hash SHA-256 de un archivo y devolverlo en Base64
   std::string b64_hash_file_SHA256(const std::string &filePath) override {
      try {
         // Construir ruta absoluta al archivo
         std::filesystem::path fullPath = std::filesystem::absolute(filePath);
         
         if (!std::filesystem::exists(fullPath))
            throw std::runtime_error("File does not exist: " + fullPath.string());
         
         // Calcular hash SHA-256
         CryptoPP::SHA256 hash;
         std::string digest;
         CryptoPP::FileSource file(fullPath.string().c_str(), true,
            new CryptoPP::HashFilter(hash,
               new CryptoPP::StringSink(digest)
            )
         );
         
         // Codificar hash en Base64
         std::string digestB64;
         CryptoPP::StringSource ssDigest(digest, true,
            new CryptoPP::Base64Encoder(
               new CryptoPP::StringSink(digestB64),
               false
            )
         );
         
         return digestB64;
      } catch (const std::exception &e) {
         std::cerr << "Error calculating SHA-256 hash for " << filePath << ": " << e.what() << std::endl;
         return "";
      }
   }
   
};