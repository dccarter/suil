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
        typedef enum { HEADER, COOKIE, NONE} From;
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

            void authorize(Jwt jwt, JwtUse use = {JwtUse::NONE, {}});

            bool authorize(String token);

            Status authenticate(Status status, String msg = {});

            inline Status authenticate(String msg = {}) {
                return Ego.authenticate(http::Unauthorized, std::move(msg));
            }

            void revoke(String redirect = {});

            inline const String& token() const {
                return Ego._actualToken;
            }

            inline const Jwt& jwt() const {
                return Ego._jwt;
            }

            bool isJwtExpired(int64 expiry) const;

        private:
            DISABLE_COPY(Context);
            DISABLE_MOVE(Context);

            JwtUse sendTok{JwtUse::NONE, {}};
            Jwt _jwt{};
            String _actualToken{};
            String _tokenHdr{};
            String _redirectUrl{};
            JwtAuthorization* jwtAuth{nullptr};
            friend struct JwtAuthorization;
            struct {
                uint8_t encode : 1;
                uint8_t requestAuth: 1;
                uint8_t __u8r6: 6;
            } __attribute__((packed)) _flags;
        };

        JwtAuthorization() = default;

        void before(Request& req, Response& resp, Context& ctx);

        void after(Request&, Response& resp, Context& ctx);

        template <typename Opts>
            requires iod::is_sio<Opts>::value
        void configure(Opts& opts) {
            /* configure expiry time */
            Ego._keyAndTokenExpiry.expiry = opts.get(sym(expires), 60);
            Ego._keyAndTokenExpiry.key = std::move(opts.get(sym(key), String{}));

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

        inline void specialize(uint32 routeId, String key, int64 expire) {
            _routeKeys.emplace(routeId, KeyWithTokenExpiry{std::move(key), expire});
        }

    private:
        DISABLE_COPY(JwtAuthorization);
        struct KeyWithTokenExpiry {
            String key;
            int64 expiry;
        };

        const KeyWithTokenExpiry& key(uint32 route) const;
        void deleteCookie(Response& resp);
        void authRequest(Response& resp, String msg = {});

        KeyWithTokenExpiry _keyAndTokenExpiry{{}, 60};
        String _authenticate{};
        String _domain{};
        String _path{};
        JwtUse _use{};
        std::unordered_map<uint32, KeyWithTokenExpiry> _routeKeys{};
    };
}
#endif //SUIL_HTTP_SERVER_JWTAUTH_HPP
