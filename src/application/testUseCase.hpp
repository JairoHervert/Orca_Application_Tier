#pragma once
#include <string>
#include <stdexcept>


// el repositorio de interes a probar
#include "../domain/repositories/IRepositoryStore.repository.hpp"

// el de cripto y el de storage
#include "../domain/repositories/IProtectRepoCrypto.repository.hpp"


class TestUseCase {
public:
   explicit TestUseCase(IRepositoryStore &repositoryStore, IProtectRepoCryptoRepository &cryptoRepo)
      : repositoryStore_(repositoryStore), cryptoRepo_(cryptoRepo) {}


   // modificar los parametros necesarios
   bool execute(const std::string &repoName) {
      // 1. Crear tar del repo
      std::filesystem::path tarPath = repositoryStore_.folderToTar(repoName, "testAlias");

      // 2. Generar clave AES
      std::string aesKeyB64 = cryptoRepo_.gen_b64_AES_GCM_Key();
      std::cout << "Generated AES Key (Base64): " << aesKeyB64 << std::endl;

      // 3. Cifrar el tar → fileOutPath en carpeta de cifrado
      std::string cipherTarPath = tarPath.string() + ".enc";
      bool cifradoOk = cryptoRepo_.cipher_AES_GCM(tarPath.string(), cipherTarPath, aesKeyB64);
      if (!cifradoOk) {
         return false;
      }

      // 4. Descifrar el tar cifrado a otro archivo tar
      std::string decipheredTarPath = tarPath.string() + "_dec.tar";
      bool descifradoOk = cryptoRepo_.decipher_AES_GCM(cipherTarPath, decipheredTarPath, aesKeyB64);
      if (!descifradoOk) {
         return false;
      }

      std::cout << "Deciphered tar path: " << decipheredTarPath << std::endl;

      // 5. Extraer el tar descifrado a una carpeta nueva
      std::filesystem::path extractedFolderPath =
         repositoryStore_.tarToFolder(decipheredTarPath);
      std::cout << "Extracted folder path: " << extractedFolderPath << std::endl;

      return true;
   }




private:
   IRepositoryStore  &repositoryStore_;
   IProtectRepoCryptoRepository &cryptoRepo_;
};