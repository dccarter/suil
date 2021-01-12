//
// Created by Mpho Mbotho on 2020-12-20.
//

#ifndef SUIL_HTTP_SERVER_JWTSESSION_HPP
#define SUIL_HTTP_SERVER_JWTSESSION_HPP

#include <suil/http/server/jwtauth.hpp>
#include <suil/http/server/redismw.hpp>
#include <suil/http/jwt.hpp>


namespace suil::http::server {

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
            }

        private:
            friend class JwtSession;
            void assertContext();
            void revoke(db::RedisClient& conn, const String& user);
            bool authorize(db::RedisClient& conn, const String& user);

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

    private:
        DISABLE_COPY(JwtSession);
        DISABLE_MOVE(JwtSession);
        void doBefore(Request& req, Response& resp, Context& ctx);
        int _sessionDb{1};
    };
}

#endif //SUIL_HTTP_SERVER_JWTSESSION_HPP
