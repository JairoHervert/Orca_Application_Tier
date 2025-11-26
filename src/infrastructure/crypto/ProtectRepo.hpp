#pragma once
#include <iostream>
#include <string>
#include <stdexcept>
#include <fstream>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/osrng.h>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>
#include <cryptopp/rsa.h>
#include "../../domain/repositories/IProtectRepoCrypto.repository.hpp"

class ProtectRepoCrypto : public IProtectRepoCryptoRepository {
public:
   explicit ProtectRepoCrypto() = default;

   std::string gen_b64_AES_GCM_Key() override {
      CryptoPP::AutoSeededRandomPool prng;
      // Generar clave AES de 32 bytes (256 bits)
      CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
      prng.GenerateBlock(key, key.size());

      // Codificar la clave en Base64
      std::string keyb64;
      CryptoPP::StringSource ss(key, key.size(), true,
         new CryptoPP::Base64Encoder(
               new CryptoPP::StringSink(keyb64),
               false
         )
      );
      return keyb64;
   }

   // Cifrar archivo (IV se guarda al inicio del archivo cifrado)
   bool cipher_AES_GCM(const std::string &filePath, const std::string &fileOutPath, const std::string &keyAES) override {      
      try {
         CryptoPP::AutoSeededRandomPool rng;

         // Decodificar clave desde Base64
         std::string decodedKey;
         CryptoPP::StringSource ssKey(keyAES, true,
               new CryptoPP::Base64Decoder(
                  new CryptoPP::StringSink(decodedKey)
               )
         );

         // Generar IV aleatorio de 12 bytes
         CryptoPP::SecByteBlock iv(12);
         rng.GenerateBlock(iv, iv.size());

         // leer el archivo original
         std::ifstream inFile(filePath, std::ios::binary);
         if (!inFile)
               throw std::runtime_error("Could not open input file: " + filePath);

         std::string plainText((std::istreambuf_iterator<char>(inFile)),
                              std::istreambuf_iterator<char>());
         inFile.close();

         // configurar cifrador AES-GCM
         CryptoPP::GCM<CryptoPP::AES>::Encryption encryptor;
         encryptor.SetKeyWithIV(
               reinterpret_cast<const CryptoPP::byte*>(decodedKey.data()),
               decodedKey.size(),
               iv, iv.size()
         );

         // cifrar el texto plano
         std::string cipherText;
         CryptoPP::StringSource ssPlain(plainText, true,
               new CryptoPP::AuthenticatedEncryptionFilter(encryptor,
                  new CryptoPP::StringSink(cipherText)
               )
         );

         // escribir IV + texto cifrado en el archivo de salida
         std::ofstream outFile(fileOutPath, std::ios::binary);
         if (!outFile)
               throw std::runtime_error("Could not open output file: " + fileOutPath);

         // escribir IV (12 bytes) al inicio
         outFile.write(reinterpret_cast<const char*>(iv.data()), iv.size());
         // escribir texto cifrado
         outFile.write(cipherText.data(), cipherText.size());
         outFile.close();

         return true;
      } catch (const std::exception &e) {
         std::cerr << "Error during AES-GCM encryption: " << e.what() << std::endl;
         return false;
      }
   }


   // Probablemente no se use y la quite
   // descifrar archivo (IV se lee del inicio del archivo cifrado)
   bool decipher_AES_GCM(const std::string &filePath, const std::string &fileOutPath, const std::string &keyAES) override {
      try {
         // Decodificar clave desde Base64
         std::string decodedKey;
         CryptoPP::StringSource ssKey(keyAES, true,
            new CryptoPP::Base64Decoder(
               new CryptoPP::StringSink(decodedKey)
            )
         );

         // Leer archivo cifrado
         std::ifstream inFile(filePath, std::ios::binary);
         if (!inFile)
            throw std::runtime_error("Could not open input file: " + filePath);

         // Extraer IV (primeros 12 bytes)
         CryptoPP::SecByteBlock iv(12);
         inFile.read(reinterpret_cast<char*>(iv.data()), iv.size());

         // Leer el resto (texto cifrado + tag)
         std::string cipherText((std::istreambuf_iterator<char>(inFile)),
                              std::istreambuf_iterator<char>());
         inFile.close();

         // Configurar descifrador AES-GCM
         CryptoPP::GCM<CryptoPP::AES>::Decryption decryptor;
         decryptor.SetKeyWithIV(
            reinterpret_cast<const CryptoPP::byte*>(decodedKey.data()),
            decodedKey.size(),
            iv, iv.size()
         );

         // Descifrar el texto cifrado
         std::string plainText;
         CryptoPP::StringSource ssCipher(cipherText, true,
            new CryptoPP::AuthenticatedDecryptionFilter(decryptor,
               new CryptoPP::StringSink(plainText)
            )
         );

         // Escribir archivo descifrado
         std::ofstream outFile(fileOutPath, std::ios::binary);
         if (!outFile)
            throw std::runtime_error("Could not open output file: " + fileOutPath);

         outFile.write(plainText.data(), plainText.size());
         outFile.close();

         return true;
      } catch (const std::exception &e) {
         std::cerr << "Error during AES-GCM decryption: " << e.what() << std::endl;
         return false;
      }
   }



   // Cifrar texto con RSA-OAEP
   std::string cipher_RSA_OAEP(const std::string &plainText, const std::string &publicKeyRSA) override {
      try {
         CryptoPP::AutoSeededRandomPool rng;

         // Decodificar clave pública desde Base64
         std::string publicKeyDER;
         CryptoPP::StringSource ssKey(publicKeyRSA, true,
               new CryptoPP::Base64Decoder(
                  new CryptoPP::StringSink(publicKeyDER)
               )
         );

         // Cargar clave pública RSA
         CryptoPP::RSA::PublicKey publicKey;
         CryptoPP::StringSource ssLoad(publicKeyDER, true);
         publicKey.Load(ssLoad);

         // Validar clave pública
         if (!publicKey.Validate(rng, 3)) {
               throw std::runtime_error("Invalid RSA public key");
         }

         // Configurar cifrador RSA-OAEP con SHA-256
         CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(publicKey);
         //CryptoPP::RSAES<CryptoPP::OAEP<CryptoPP::SHA256>>::Encryptor encryptor(publicKey);


         // Cifrar el texto plano
         std::string cipherText;
         CryptoPP::StringSource ssPlain(plainText, true,
               new CryptoPP::PK_EncryptorFilter(rng, encryptor,
                  new CryptoPP::StringSink(cipherText)
               )
         );

         // Codificar resultado en Base64
         std::string cipherTextBase64;
         CryptoPP::StringSource ssCipher(cipherText, true,
               new CryptoPP::Base64Encoder(
                  new CryptoPP::StringSink(cipherTextBase64),
                  false  // sin saltos de línea
               )
         );

         return cipherTextBase64;

      } catch (const std::exception &e) {
         std::cerr << "Error during RSA-OAEP encryption: " << e.what() << std::endl;
         return "";
      }
   }

private:
   // Aquí puedes agregar métodos privados para ayudar en la implementación
};