//
// Created by Mpho Mbotho on 2020-11-24.
//

#ifndef SUILNETWORK_AUTH_HPP
#define SUILNETWORK_AUTH_HPP

#include <suil/base/string.hpp>

namespace suil::net::smtp {

    enum class AuthMechanism {
        Plain,
        Login,
        DigestMD5,
        MD5,
        CramMD5,
        PlainToken,
        OAuthBearer,
        Unknown
    };

    AuthMechanism parseAuthMechanism(const String& str);
    String toString(AuthMechanism mechanism);

}

#endif //SUILNETWORK_AUTH_HPP
