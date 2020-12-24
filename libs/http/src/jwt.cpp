//
// Created by Mpho Mbotho on 2020-12-19.
//

#include "suil/http/jwt.hpp"

#include <suil/base/base64.hpp>
#include <suil/base/hash.hpp>

#include <openssl/sha.h>

namespace suil::http {

    Jwt::Jwt(String typ)
        : _header{std::move(typ)}
    {}

    bool Jwt::decode(Jwt& jwt, const String& jstr, const String& secret, bool fault)
    {
        // expect header.payload.signature

        // find start of signature
        auto pos = jstr.rfind('.');
        if (pos == String::npos) {
            // invalid jwt
            if (fault)
                throw JwtDecodeError("invalid token");
            return false;
        }

        // get each part
        auto parts = jstr.parts(".");
        if (parts.size() != 3) {
            // invalid jwt
            if (fault)
                throw JwtDecodeError("invalid token");
            return false;
        }

        auto data = jstr.substr(0, pos);
        auto signature = suil::SHA_HMAC256(secret, data, true);
        if (signature != parts[2]) {
            // Token might have been changed
            return false;
        }

        auto header = Base64::urlDecode(parts[0]);
        json::decode(header, jwt._header);
        auto payload = Base64::urlDecode(parts[1]);
        json::decode(payload, jwt._payload);

        return true;
    }

    bool Jwt::verify(const String& jstr, const String& secret, bool fault)
    {
        // find start of signature
        auto pos = jstr.rfind('.');
        if (pos == String::npos) {
            // invalid jwt
            if (fault)
                throw JwtDecodeError("invalid token");
            return false;
        }

        auto data = jstr.substr(0, pos++);
        auto signature = suil::SHA_HMAC256(secret, data, true);
        if (signature != jstr.substr(pos)) {
            // Token might have been changed
            return false;
        }

        return true;
    }

    String Jwt::encode(const String& secret) const
    {
        // 1. Json(header).Json(payload)
        auto header = json::encode(Ego._header);
        auto payload = json::encode(Ego._payload);

        // 2. Base64(header).Base64(payload)
        auto maxSize = (2 + header.size() + payload.size() + SHA256_DIGEST_LENGTH);
        maxSize = (maxSize >> 2) + (maxSize >> 3) + maxSize + 2;
        Buffer tmp{maxSize};
        Base64::urlEncode(tmp, reinterpret_cast<const uint8*>(header.data()), header.size());
        tmp.append('.');
        Base64::urlEncode(tmp, reinterpret_cast<const uint8*>(payload.data()), payload.size());

        // 3. #2.HMAC256(#2)
        auto signature = suil::SHA_HMAC256(secret, String{tmp, false, }, true);
        tmp.append('.');
        tmp << signature;

        return String{tmp};
    }

    void Jwt::roles(const std::vector<String>& roles)
    {
        auto rs = Ego._payload.claims["roles"];
        if (rs.empty())
            Ego._payload.claims.set("roles", json::Object{json::Arr, roles});
        else for(auto& r: roles)
                rs.push(r);

    }
}