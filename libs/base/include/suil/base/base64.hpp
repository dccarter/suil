//
// Created by Mpho Mbotho on 2020-10-07.
//

#ifndef SUIL_BASE_BASE64_HPP
#define SUIL_BASE_BASE64_HPP

#include "suil/base/string.hpp"

namespace suil::Base64 {

    void encode(Buffer& ob, const uint8_t *, size_t);

    String encode(const uint8_t *, size_t);

    static String encode(const String &str) {
        return encode((const uint8_t *) str.data(), str.size());
    }

    static String encode(const std::string &str) {
        return encode((const uint8_t *) str.data(), str.size());
    }

    void decode(Buffer& ob, const uint8_t *in, size_t len);

    String decode(const uint8_t *in, size_t len);

    static String decode(std::string_view &sv) {
        return std::move(decode((const uint8_t *) sv.data(), sv.size()));
    }

    static String decode(const String &zc) {
        return std::move(decode((const uint8_t *) zc.data(), zc.size()));
    }

    void urlEncode(Buffer& ob, const uint8 *b, size_t len);

    String urlEncode(const uint8_t *, size_t);

    static String urlEncode(const String &str) {
        return urlEncode((const uint8_t *) str.data(), str.size());
    }

    static String urlEncode(const std::string &str) {
        return urlEncode((const uint8_t *) str.data(), str.size());
    }

    void urlDecode(Buffer& ob, const uint8_t *in, size_t len);

    String urlDecode(const uint8_t *in, size_t len);

    static String urlDecode(std::string_view &sv) {
        return std::move(urlDecode((const uint8_t *) sv.data(), sv.size()));
    }

    static String urlDecode(const String &zc) {
        return std::move(urlDecode((const uint8_t *) zc.data(), zc.size()));
    }
}
#endif //SUIL_BASE_BASE64_HPP
