#pragma once
#include <optional>
#include <string>

class IProtectRepoCryptoRepository {
public:
   virtual ~IProtectRepoCryptoRepository() = default;

   virtual std::string gen_b64_AES_GCM_Key() = 0;

   virtual bool cipher_AES_GCM(const std::string &filePath, const std::string &fileOutPath, const std::string &keyAES) = 0;

   // esta probablemente no se use y la quite
   virtual bool decipher_AES_GCM(const std::string &filePath, const std::string &fileOutPath, const std::string &keyAES) = 0;

   virtual std::string cipher_RSA_OAEP(const std::string &plainText, const std::string &publicKeyRSA) = 0;

};