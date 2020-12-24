//
// Created by Mpho Mbotho on 2020-10-08.
//

#ifndef SUIL_BASE_ENCRYPTION_HPP
#define SUIL_BASE_ENCRYPTION_HPP

#include "suil/base/string.hpp"

namespace suil {

    Data AES_EncryptBin(const String &secret, const uint8_t *data, size_t size);

    String AES_Encrypt(const String &secret, const uint8_t *data, size_t size, bool b64 = true);

    template <DataBuf T>
    String AES_Encrypt(const String& secret, const T& data, bool b64 = true) {
        return AES_Encrypt(
                secret,
                reinterpret_cast<const uint8_t *>(data.data()),
                data.size(),
                b64);
    }

    Data AES_DecryptBin(const String& secret, const uint8_t *data, size_t size);

    template <DataBuf T>
    Data AES_Decrypt(const String& secret, const T& data, bool b64 = true) {
        auto dec = suil::bytes(
                reinterpret_cast<const uint8_t*>(data.data()),
                data.size(),
                b64);
        return AES_DecryptBin(secret, dec.data(), dec.size());
    }

    Data RSA_EncryptBin(const String& publicKey, const uint8_t* data, size_t size);

    String RSA_Encrypt(const String& publicKey, const uint8_t* data, size_t size, bool b64 = true);

    template <DataBuf D>
    String RSA_Encrypt(const String& publicKey, const D& data, bool b64 = true) {
        return RSA_Encrypt(publicKey, static_cast<const uint8_t *>(data.data()), data.size(), b64);
    }

    Data RSA_DecryptBin(const String& privateKey, const uint8_t *data, size_t size);

    template <DataBuf D>
    Data RSA_Decrypt(const String& privateKey, const D& data, bool b64 = true) {
        auto dec = suil::bytes((const uint8_t *)data.data(), data.size(), b64);
        return RSA_DecryptBin(privateKey, dec.data(), dec.size());
    }
}
#endif //SUIL_BASE_ENCRYPTION_HPP
