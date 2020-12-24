//
// Created by Mpho Mbotho on 2020-12-20.
//

#ifndef SUIL_HTTP_SERVER_JWTAUTH_HPP
#define SUIL_HTTP_SERVER_JWTAUTH_HPP

#include <suil/http/server/request.hpp>
#include <suil/http/server/response.hpp>
#include <suil/http/jwt.hpp>

namespace suil::http::server {

    define_log_tag(JWT_AUTH);

    struct JwtUse {
        typedef enum { HEADER, COOKIE} From;
        JwtUse()
            : use(From::HEADER)
        {}

        JwtUse(From from, String key)
            : use(from),
              key(std::move(key))
        {}

        From use;
        String key{"Authorization"};
    };

    class JwtAuthorization: public LOGGER(JWT_AUTH) {
    public:
        struct Context {
            Context();

            void authorize(Jwt jwt);

            bool authorize(String token);

            Status authenticate(Status status, String msg = {});

            inline Status authenticate(String msg = {}) {
                return Ego.authenticate(http::Unauthorized, std::move(msg));
            }

            void logout(String redirect = {});

            inline const String& token() const {
                return Ego._actualToken;
            }

            inline const Jwt& jwt() const {
                return Ego._jwt;
            }

        private:
            DISABLE_COPY(Context);
            DISABLE_MOVE(Context);

            Jwt _jwt{};
            String _actualToken{};
            String _tokenHdr{};
            String _redirectUrl{};
            JwtAuthorization* jwtAuth{nullptr};
            friend struct JwtAuthorization;
            struct {
                uint8_t sendTok : 1;
                uint8_t encode : 1;
                uint8_t requestAuth: 1;
                uint8_t __u8r5: 5;
            } __attribute__((packed)) _flags;
        };

        JwtAuthorization() = default;

        void before(Request& req, Response& resp, Context& ctx);

        void after(Request&, Response& resp, Context& ctx);

        template <typename Opts>
            requires iod::is_sio<Opts>::value
        void configure(Opts& opts) {
            /* configure expiry time */
            Ego._expiry = opts.get(sym(expires), 3600);
            Ego._encodeKey = std::move(opts.get(sym(key), String{}));

            /* configure authenticate header string */
            Ego._authenticate = std::move(opts.get(sym(realm), String{}));

            /* configure domain */
            Ego._domain = std::move(opts.get(sym(domain), String()));
            /* configure domain */
            Ego._path   = std::move(opts.get(sym(path), String()));

            if (opts.has(sym(jwtTokenUse))) {
                Ego._use = std::move(opts.get(sym(jwtTokenUse), Ego._use));
            }
        }

        template <typename... Opts>
        inline void setup(Opts&&... opts) {
            auto options = iod::D(std::forward<Opts>(opts)...);
            configure(options);
        }

    private:
        DISABLE_COPY(JwtAuthorization);

        void deleteCookie(Response& resp);
        void authRequest(Response& resp, String msg = {});

        uint64 _expiry{900};
        String _encodeKey{};
        String _authenticate{};
        String _domain{};
        String _path{};
        JwtUse _use{};
    };
}
#endif //SUIL_HTTP_SERVER_JWTAUTH_HPP
