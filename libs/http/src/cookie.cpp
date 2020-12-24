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
}