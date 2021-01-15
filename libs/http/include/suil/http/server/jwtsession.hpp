//
// Created by Mpho Mbotho on 2020-12-20.
//

#ifndef SUIL_HTTP_SERVER_JWTSESSION_HPP
#define SUIL_HTTP_SERVER_JWTSESSION_HPP

#include <suil/http/server/jwtauth.hpp>
#include <suil/http/server/redismw.hpp>
#include <suil/http/server/rules.hpp>
#include <suil/http/jwt.hpp>


namespace suil::http::server {

    DECLARE_EXCEPTION(JwtSessesionExecption);

    class JwtSession {
    public:
        // specify dependencies
        using Depends = std::tuple<JwtAuthorization, RedisMiddleware>;

        struct Context {
            bool authorize(Jwt jwt);

            inline bool authorize(const String& user) {
                assertContext();
                scoped(conn, redisContext->conn(jwtSession->_sessionDb));
                return Ego.authorize(conn, user);
            }

            inline void revoke(const Jwt& jwt) {
                Ego.revoke(jwt.aud());
            }

            inline void revoke(const String& user) {
                assertContext();
                scoped(conn, redisContext->conn(jwtSession->_sessionDb));
                revoke(conn, user);
            }

            const String& token() const {
                return _token;
            }

        private:
            friend class JwtSession;
            void assertContext();
            void revoke(db::RedisClient& conn, const String& user);
            bool authorize(db::RedisClient& conn, const String& user);

            String _token{};
            RedisMiddleware::Context *redisContext{nullptr};
            JwtAuthorization::Context *jwtAuth{nullptr};
            JwtSession                *jwtSession{nullptr};
        };

        JwtSession() = default;

        template <typename Contexts>
        void before(Request& req, Response& resp, Context& ctx, Contexts& ctxs) {
            ctx.jwtSession = this;
            ctx.jwtAuth = &ctxs.template get<JwtAuthorization>();
            ctx.redisContext = &ctxs.template get<RedisMiddleware>();
            doBefore(req, resp, ctx);
        }

        void after(Request& req, Response&, Context& ctx);

        template <typename... Opts>
        void setup(Opts&&... options)
        {
            auto opts = iod::D(std::forward<Opts>(options)...);
            _refreshTokenKey = opts.get(var(refreshTokenKey), String{});
            requireRefreshTokenKey();

            _sessionDb = opts.get(var(jwtSessionDb), 1);
            _refreshTokenExpiry = opts.get(var(expires), 1296000);
        }

        template <typename Ep, typename Opt>
            requires (iod::is_symbol<typename Opt::left_t>::value &&
                      std::is_same_v<typename Opt::left_t, Sym(onTokenRefresh)>)
        DynamicRule& refresh(Ep& ep, Opt&& opt)
        {
            requireRefreshTokenKey();
            // Preconfigure a route that will be used to setup a refresh token
            auto& route = ep(opt.right);

            route(Method::Get, Method::Options)
                .attrs(opt(Authorize, true),
                       opt(ReplyType, "application/json"));
            _refreshTokenRoute = route.id();

            ep.template middleware<JwtAuthorization>().specialize(
                    _refreshTokenRoute,
                    _refreshTokenKey.peek(),
                    _refreshTokenExpiry);
            return route;
        }

        template <typename Ep, typename Opt>
            requires (iod::is_symbol<typename Opt::left_t>::value &&
                      std::is_same_v<typename Opt::left_t, Sym(onTokenRevoke)>)
        DynamicRule& refresh(Ep& ep, Opt&& opt)
        {
            requireRefreshTokenKey();
            // Preconfigure a route that will be used to setup a refresh token
            auto& route = ep(opt.right);

            route(Method::Delete, Method::Options)
                .attrs(opt(Authorize, true));
            _revokeTokenRoute = route.id();

            ep.template middleware<JwtAuthorization>().specialize(
                    _revokeTokenRoute,
                    _refreshTokenKey.peek(),
                    _refreshTokenExpiry);
            return route;
        }

    private:
        DISABLE_COPY(JwtSession);
        DISABLE_MOVE(JwtSession);
        void doBefore(Request& req, Response& resp, Context& ctx);
        inline bool isSpecialRoute(uint32 id) {
            return (id == _revokeTokenRoute) or (id == _refreshTokenRoute);
        }

        inline void requireRefreshTokenKey() {
            if (!_refreshTokenKey) {
                throw JwtSessesionExecption("A refresh token key is needed for issues JWT accept tokens");
            }
        }

        int _sessionDb{1};
        int64 _refreshTokenExpiry{1296000}; // 15 days
        String _refreshTokenKey{};
        uint32 _refreshTokenRoute{0};
        uint32 _revokeTokenRoute{0};
    };
}

#include <suil/db/redis.hpp>
#endif //SUIL_HTTP_SERVER_JWTSESSION_HPP
