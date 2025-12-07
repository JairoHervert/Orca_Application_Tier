#pragma once
#include <iostream>
#include <string>
#include <stdexcept>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/sha.h>
#include <cryptopp/base64.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
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

   // Verificar firma ECDSA (P-256) de un archivo
   bool verify_signature_ecdsa_p256(const std::string &filePath, const std::string &signatureB64, const std::string &publicKeyB64) override {
      try {
         CryptoPP::AutoSeededRandomPool rng;

         // 1. Decodificar la clave pública desde Base64
         std::string publicKeyDER;
         CryptoPP::StringSource ssKey(publicKeyB64, true,
            new CryptoPP::Base64Decoder(
               new CryptoPP::StringSink(publicKeyDER)
            )
         );

         // 2. Cargar la clave pública ECDSA
         CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicKey;
         CryptoPP::StringSource ssLoad(publicKeyDER, true);
         publicKey.Load(ssLoad);

         // 3. Validar la clave pública
         if (!publicKey.Validate(rng, 3)) {
            std::cerr << "Invalid ECDSA public key" << std::endl;
            return false;
         }

         // 4. Decodificar la firma desde Base64
         std::string signature;
         CryptoPP::StringSource ssSig(signatureB64, true,
            new CryptoPP::Base64Decoder(
               new CryptoPP::StringSink(signature)
            )
         );

         // 5. Calcular el hash SHA-256 del archivo
         std::filesystem::path fullPath = std::filesystem::absolute(filePath);
         if (!std::filesystem::exists(fullPath)) {
            std::cerr << "File does not exist: " << fullPath << std::endl;
            return false;
         }

         CryptoPP::SHA256 hash;
         std::string digest;
         CryptoPP::FileSource file(fullPath.string().c_str(), true,
            new CryptoPP::HashFilter(hash,
               new CryptoPP::StringSink(digest)
            )
         );

         // 6. Verificar la firma
         CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Verifier verifier(publicKey);
         
         bool result = verifier.VerifyMessage(
            reinterpret_cast<const CryptoPP::byte*>(digest.data()),
            digest.size(),
            reinterpret_cast<const CryptoPP::byte*>(signature.data()),
            signature.size()
         );

         if (result) {
            std::cout << "✓ Signature verified for: " << filePath << std::endl;
         } else {
            std::cerr << "✗ Invalid signature for: " << filePath << std::endl;
         }

         return result;

      } catch (const CryptoPP::Exception &e) {
         std::cerr << "CryptoPP error verifying signature: " << e.what() << std::endl;
         return false;
      } catch (const std::exception &e) {
         std::cerr << "Error verifying signature: " << e.what() << std::endl;
         return false;
      }
   }


   bool verify_signature_ecdsa_p256_over_string(
      const std::string &message,
      const std::string &signatureB64,
      const std::string &publicKeyB64) override
   {
      try {
         CryptoPP::AutoSeededRandomPool rng;

         // 1. Decodificar clave pública DER desde Base64
         std::string publicKeyDER;
         CryptoPP::StringSource ssKey(publicKeyB64, true,
            new CryptoPP::Base64Decoder(
               new CryptoPP::StringSink(publicKeyDER)
            )
         );

         CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey publicKey;
         CryptoPP::StringSource ssLoad(publicKeyDER, true);
         publicKey.Load(ssLoad);

         if (!publicKey.Validate(rng, 3)) {
            std::cerr << "Invalid ECDSA public key (over_string)\n";
            return false;
         }

         // 2. Decodificar firma desde Base64
         std::string signature;
         CryptoPP::StringSource ssSig(signatureB64, true,
            new CryptoPP::Base64Decoder(
               new CryptoPP::StringSink(signature)
            )
         );

         // 3. Verificar la firma sobre *message* tal cual (texto Base64)
         CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Verifier verifier(publicKey);

         bool result = verifier.VerifyMessage(
            reinterpret_cast<const CryptoPP::byte*>(message.data()),
            message.size(),
            reinterpret_cast<const CryptoPP::byte*>(signature.data()),
            signature.size()
         );

         if (result) {
            std::cout << "✓ Signature over string verified\n";
         } else {
            std::cerr << "✗ Invalid signature over string\n";
         }

         return result;
      }
      catch (const CryptoPP::Exception &e) {
         std::cerr << "CryptoPP error verifying signature over string: " << e.what() << std::endl;
         return false;
      }
      catch (const std::exception &e) {
         std::cerr << "Error verifying signature over string: " << e.what() << std::endl;
         return false;
      }
   }
};