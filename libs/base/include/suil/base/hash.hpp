//
// Created by Mpho Mbotho on 2020-10-08.
//

#ifndef SUIL_BASE_HASH_HPP
#define SUIL_BASE_HASH_HPP

#include "suil/base/string.hpp"
#include "suil/base/buffer.hpp"

namespace suil {

    String SHA_HMAC256(const String&, const uint8_t *, size_t, bool b64 = false);

    template <DataBuf T>
    static inline String SHA_HMAC256(const String &secret, const T& msg, bool b64 = false) {
        return SHA_HMAC256(secret, (const uint8_t *) msg.data(), msg.size(), b64);
    }

    static inline String SHA_HMAC256(String &secret, Buffer &msg, bool b64 = false) {
        return SHA_HMAC256(secret, (const uint8_t *) msg.data(), msg.size(), b64);
    }

    String MD5(const uint8_t *, size_t);

    inline String MD5(const char *str) {
        return MD5(reinterpret_cast<const uint8_t*>(str), strlen(str));
    }

    template <DataBuf T>
    inline String MD5(const T &data) {
        return MD5(
                reinterpret_cast<const uint8_t *>(data.data()),
                data.size());
    }

    String SHA256(const uint8_t *data, size_t len, bool b64 = false);

    template <DataBuf T>
    static inline String SHA256(const T &data, bool b64 = false) {
        return SHA256(
                reinterpret_cast<const uint8_t *>(data.data()),
                data.size(),
                b64);

    }

    String SHA512(const uint8_t *data, size_t len, bool b64 = false);

    template <DataBuf T>
    static inline String SHA512(const T &data, bool b64 = false) {
        return SHA512(
                reinterpret_cast<const uint8_t *>(data.data()),
                data.size(),
                b64);
    }

}
#endif //SUIL_BASE_HASH_HPP
