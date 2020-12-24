//
// Created by Mpho Mbotho on 2020-10-12.
//

#include "suil/base/encryption.hpp"
#include "suil/base/base64.hpp"
#include "suil/base/logging.hpp"

#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>

namespace suil {


    String AES_Encrypt(const String &secret, const uint8_t *data, size_t size, bool b64)
    {
#define logError() serror("AES_Encrypt %s", ERR_error_string(ERR_get_error(), nullptr));
        auto bin = AES_EncryptBin(secret, data, size);
        String res{nullptr};
        if (b64) {
            return Base64::encode(bin.data(), bin.size());
        }
        else {
            return hexstr(bin.data(), bin.size());
        }
#undef logError
    }

    Data AES_EncryptBin(const String &pass, const uint8_t *data, size_t size)
    {
#define logError() serror("AES_EncryptBin %s", ERR_error_string(ERR_get_error(), nullptr));
        uint8_t key[32] = {0};
        uint8_t iv[16] = {0};

        EVP_BytesToKey(EVP_aes_256_cbc(),
                       EVP_sha1(),
                       nullptr,
                       (const uint8_t *) pass.data(),
                       static_cast<int>(pass.size()),
                       1,
                       key,
                       iv);

        EVP_CIPHER_CTX *ctx{EVP_CIPHER_CTX_new()};
        if (ctx == nullptr) {
            logError();
            return {};
        }

        if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv)) {
            logError();
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        auto cipherLen = static_cast<int>(size + AES_BLOCK_SIZE);
        auto cipherText = static_cast<uint8_t *>(malloc(static_cast<size_t>(cipherLen)));
        if (cipherText == nullptr) {
            serror("AES_Encrypt malloc(%d) failed: %s", cipherLen, errno_s);
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
        if(1 != EVP_EncryptUpdate(ctx, cipherText, &cipherLen, data, static_cast<int>(size))) {
            logError();
            free(cipherText);
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        int len{0};
        if(1 != EVP_EncryptFinal_ex(ctx, cipherText + cipherLen, &len)) {
            logError();
            free(cipherText);
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
        cipherLen += len;
        EVP_CIPHER_CTX_free(ctx);
        return Data{cipherText, static_cast<size_t>(cipherLen), true};
#undef logError
    }

    Data AES_DecryptBin(const String &secret, const uint8_t *data, size_t size)
    {
#define logError() serror("AES_DecryptBin %s", ERR_error_string(ERR_get_error(), nullptr));
        uint8_t key[32] = {0};
        uint8_t iv[16] = {0};

        EVP_BytesToKey(EVP_aes_256_cbc(),
                       EVP_sha1(),
                       nullptr,
                       (const uint8_t *)secret.data(),
                       static_cast<int>(secret.size()),
                       1,
                       key,
                       iv);

        EVP_CIPHER_CTX *ctx{EVP_CIPHER_CTX_new()};
        if (ctx == nullptr) {
            logError();
            return {};
        }

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
            logError();
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        int p_len{static_cast<int>(size)}, f_len{0};
        auto *plainText = static_cast<unsigned char *>(malloc(size));
        if (plainText == nullptr) {
            serror("AES_Decrypt malloc(%d) failed: %s", p_len, errno_s);
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (1 != EVP_DecryptUpdate(ctx, plainText, &p_len, data, static_cast<int>(size))) {
            logError();
            free(plainText);
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
        if (1 != EVP_DecryptFinal_ex(ctx, plainText+p_len, &f_len)) {
            logError();
            free(plainText);
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        p_len += f_len;
        plainText[p_len++] = '\0';

        EVP_CIPHER_CTX_free(ctx);
        return Data{plainText, static_cast<size_t>(p_len), true};
#undef logError
    }

    Data RSA_EncryptBin(const String &publicKey, const uint8_t *data, size_t size)
    {
        return {};
    }

    Data RSA_DecryptBin(const String &privateKey, const uint8_t *data, size_t size)
    {
        return {};
    }
}

