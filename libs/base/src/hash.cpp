//
// Created by Mpho Mbotho on 2020-10-08.
//

#include "suil/base/hash.hpp"
#include "suil/base/base64.hpp"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/sha.h>


namespace suil {

    String MD5(const uint8_t *data, size_t len) {
        if (data == nullptr)
            return String{};

        uint8_t RAW[MD5_DIGEST_LENGTH];
        ::MD5(data, len, RAW);
        return std::move(hexstr(RAW, MD5_DIGEST_LENGTH));
    }

    String SHA_HMAC256(const String &secret, const uint8_t *data, size_t len, bool b64) {
        if (data == nullptr)
            return String{};

        uint8_t *result = ::HMAC(EVP_sha256(), secret.data(), (int) secret.size(),
                               data, len, nullptr, nullptr);
        if (b64) {
            return Base64::urlEncode(result, SHA256_DIGEST_LENGTH);
        }
        else {
            return std::move(hexstr(result, SHA256_DIGEST_LENGTH));
        }
    }

    String SHA256(const uint8_t *data, size_t len, bool b64) {
        if (data == nullptr)
            return String{nullptr};

        uint8_t *result = ::SHA256(data, len, nullptr);
        if (b64) {
            return Base64::urlEncode(hexstr(result, SHA256_DIGEST_LENGTH));
        }
        else {
            return hexstr(result, SHA256_DIGEST_LENGTH);
        }
    }

    String SHA512(const uint8_t *data, size_t len, bool b64) {
        if (data == nullptr)
            return String{nullptr};

        uint8_t *result = ::SHA512(data, len, nullptr);
        if (b64) {
            return Base64::urlEncode(hexstr(result, SHA512_DIGEST_LENGTH));
        }
        else {
            return hexstr(result, SHA512_DIGEST_LENGTH);
        }
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

namespace sb = suil;

TEST_CASE("Using hashing functions", "[SHA_HAM256][MD5][SHA256][SHA512]")
{
    WHEN("MD5 hashing") {
        sb::String data[][2] = {
            {"The quick brown fox",  "a2004f37730b9445670a738fa0fc9ee5"},
            {"Jumped over the fire", "b6283454ffe0eb2b97c8d8d1b94062dc"},
            {"Simple world",         "1b5b2eb127a6f47cad4ddcde1b5a5205"},
            {"Suilman of SuilCity",  "f6ce722986cb80b57f153faf0ccf51f1"}
        };

        for (auto &i : data) {
            sb::String out = sb::MD5(i[0]);
            REQUIRE(out.compare(i[1]) == 0);
        }
    }

    WHEN("HMAC_SHA256 hashing") {
        // 8559f3e178983e4e83b20b5688f335c133c88015c51f3a9f7fcfde5cd4f613dc
        sb::String sentence =
                "The quick brown fox jumped over the lazy dog.\n"
                "The lazy dog just lied there thinking 'the world is \n"
                "circle, i'll get that quick brown fox'";
        sb::String secret{"awfulSecret12345"};

        // compute the hash
        sb::String hashed = sb::SHA_HMAC256(secret, sentence);
        REQUIRE(hashed.compare("8559f3e178983e4e83b20b5688f335c133c88015c51f3a9f7fcfde5cd4f613dc") == 0);
    }
}

#endif
