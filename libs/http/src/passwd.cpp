//
// Created by Mpho Mbotho on 2020-12-19.
//

#include "suil/http/passwd.hpp"

#include <suil/base/buffer.hpp>

#include <openssl/evp.h>
#include <openssl/sha.h>


namespace suil::http {

    String Pbkdf2Sha1Hash::operator()(const String& passwd, const String& salt)
    {
        auto tmp = suil::catstr(GlobalSalt, salt);
        uint8 out[SHA256_DIGEST_LENGTH] = {0};

        auto rc = PKCS5_PBKDF2_HMAC_SHA1(
                        passwd.data(),
                        passwd.size(),
                        reinterpret_cast<const uint8*>(tmp.data()),
                        tmp.size(),
                        1000,
                        SHA_DIGEST_LENGTH,
                        out);
        if (rc) {
            return suil::hexstr(out, SHA256_DIGEST_LENGTH);
        }

        return {};
    }

#define isspecial(a) (((a) > 31 && (a) < 48) || ((a) > 58 && (a) < 65) || ((a) > 90 && (a) < 97) || ((a) > 123 && (a) < 127))
    bool PasswdValidator::operator()(Buffer& err, const String& passwd, const String& sep) const
    {
        if (passwd.size() < MinChars) {
            if (sep)
                err << "Password too short, at least '" << MinChars << "' characters required";
            return false;
        }
        if (passwd.size() > MaxChars) {
            if (sep)
                err << "Password too long, at most '" << MaxChars << "' characters supported";
            return false;
        }
        if (!Upper && !Lower && !Special && !Numbers)
            return true;

        int u{0}, l{0}, s{0}, n{0};
        for (const auto& c: passwd) {
            if (isspecial(c)) s++;
            else if (isalpha(c))
                if (isupper(c)) u++;
                else l++;
            else if (isdigit(c)) n++;
        }

        auto failed = u != 0 || l != 0 || s != 0 || n != 0;
        if (!sep) {
            return failed;
        }

        if (Upper && u < Upper) {
            err << "Password invalid, requires at least '"
                << Upper << "' uppercase characters";
        }
        if (Lower && l < Lower) {
            err << (err.empty() ? "" : sep) << "Password invalid, requires at least '" << Lower
                << "' lowercase characters";
        }
        if (Special && s < Special) {
            err << (err.empty() ? "" : sep) << "Password invalid, requires at least '" << Special
                << "' special characters";
        }
        if (Numbers && n < Numbers) {
            err << (err.empty() ? "" : sep) << "Password invalid, requires at least '" << Numbers
                << "' digits";
        }

        return failed;
    }

    bool PasswdValidator::operator()(const String& passwd) const
    {
        Buffer ob{0};
        return Ego(ob, passwd, {nullptr});
    }
#undef isspecial

}