//
// Created by Mpho Mbotho on 2020-12-17.
//

#include "suil/http/cookie.hpp"

#include <suil/base/buffer.hpp>

namespace suil::http {

    Cookie::Cookie(String name)
        : _name{std::move(name)}
    {}

    Cookie::Cookie(String name, String value)
        : _name{std::move(name)},
          _value{std::move(value)}
    {}

    bool Cookie::encode(Buffer& dst) const
    {
        if (Ego.empty() or Ego._value.empty()) {
            return false;
        }

        dst.reserve(127);
        dst << Ego._name << '=' << Ego._value;

        if (Ego._domain) {
            // append domain
            dst << "; Domain=" << Ego._domain;
        }

        if (Ego._path) {
            // append path
            dst << "; Path=" << Ego._domain;
        }

        if (Ego._secure) {
            // mark cookie as secure
            dst << "; Secure";
        }

        if (Ego._maxAge > 0) {
            // append max age
            dst.appendf("; Max-Age=%lu", Ego._maxAge);
        }

        if (Ego._expires > 0) {
            // add cookie expiry
            dst << "; Expires=";
            dst.append(Ego._expires, nullptr);
        }
        return true;
    }

    Cookie Cookie::parse(const String& cookieStr)
    {
        auto parseKeyPair = [&](const String& part) -> std::pair<String, String> {
            auto eq = cookieStr.find('=');
            if (eq == String::npos) {
                return {part.trim(' '), {}};
            }
            auto name = cookieStr.substr(0, eq).strip(' ', true);
            auto val = cookieStr.substr(eq+1).strip(' ', true);
            return {std::move(name), std::move(val)};
        };

        auto pos = cookieStr.find(';');
        if (pos == String::npos) {
            auto [name, val] = parseKeyPair(cookieStr);
            return Cookie{std::move(name), std::move(val)};
        }

        auto [name, val] = parseKeyPair(cookieStr.substr(0, pos));
        Cookie cookie{std::move(name), std::move(val)};
        // @TODO Parse reset of cookie
        return std::move(cookie);
    }
}