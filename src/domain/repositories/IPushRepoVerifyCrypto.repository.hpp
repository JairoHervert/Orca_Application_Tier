#pragma once
#include <optional>
#include <string>

class IPushRepoCryptoRepository {
public:
   virtual ~IPushRepoCryptoRepository() = default;

   virtual std::string b64_hash_file_SHA256(const std::string &filePath) = 0;

   virtual bool verify_signature_ecdsa_p256(const std::string &filePath, const std::string &signatureB64, const std::string &publicKeyB64) = 0;
};