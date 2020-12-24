//
// Created by Mpho Mbotho on 2020-12-19.
//

#ifndef SUIL_HTTP_PASSWD_HPP
#define SUIL_HTTP_PASSWD_HPP

#include <suil/http/server.scc.hpp>

#include <suil/base/string.hpp>

namespace suil::http {

    struct Pbkdf2Sha1Hash final {
        Pbkdf2Sha1Hash(const String& salt)
            : GlobalSalt{salt}
        {}

        String operator()(const String& passwd, const String& salt);
        const String& GlobalSalt;
    };

    struct PasswdValidator final {
        template <typename ...Opts>
        PasswdValidator(Opts&&... opts)
        {
            suil::applyConfig(Ego, std::forward<Opts>(opts)...);
        }

        DISABLE_COPY(PasswdValidator);
        DISABLE_MOVE(PasswdValidator);

        bool operator()(const String& passwd) const;
        bool operator()(Buffer& err, const String& passwd, const String& sep = "\n") const;

    private:
        int MinChars{6};
        int MaxChars{32};
        int Numbers{0};
        int Special{0};
        int Upper{0};
        int Lower{0};
    };
}
#endif //SUIL_HTTP_PASSWD_HPP
