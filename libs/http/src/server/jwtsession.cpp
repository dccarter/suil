//
// Created by Mpho Mbotho on 2020-12-20.
//

#include "suil/http/server/jwtsession.hpp"

namespace suil::http::server {

    bool JwtSession::Context::authorize(Jwt jwt)
    {
        assertContext();
        // pass token to JWT authorization middleware
        jwtAuth->authorize(std::move(jwt));
        // store generated token in database
        scoped(conn, redisContext->conn(jwtSession->_sessionDb));
        if (!conn.set(jwtAuth->jwt().aud(), jwtAuth->token())) {
            // saving jwt authentication failed
            jwtAuth->authenticate("Creating session failed");
            return false;
        }

        auto expires = jwtAuth->jwt().exp();
        if (expires > 0) {
            // when token expires, it should be removed from database
            conn.expire(jwtAuth->jwt().aud(), expires-time(nullptr));
        }

        return true;
    }

    void JwtSession::Context::revoke(db::RedisClient& conn, const String& user)
    {
        // remove stored token and any keys associated with the user
        conn.del(user);
        auto keys = conn.keys(suil::catstr("*", user));
        for (auto& key : keys) {
            conn.del(key);
        }
        jwtAuth->logout();
    }

    bool JwtSession::Context::authorize(db::RedisClient& conn, const String& user)
    {
        if (!conn.exists(user)) {
            // session does not exist
            return false;
        }

        auto token = conn.get<String>(user);
        if (!jwtAuth->authorize(token)) {
            // token has been checked and it's invalid
            Ego.revoke(conn, user);
            return false;
        }

        return true;
    }

    void JwtSession::Context::assertContext()
    {
        if (jwtSession == nullptr) {
            throw UnsupportedOperation(
                    "JwtSession middleware can only be used in routes or "
                    "after the JwtAuthorization and RedisMiddleware");
        }
    }

    void JwtSession::doBefore(Request& req, Response& resp, Context& ctx)
    {
        if (!ctx.jwtAuth->token().empty()) {
            // request has authorization token, ensure it is still valid
            scoped(conn, ctx.redisContext->conn(Ego._sessionDb));
            auto res = conn("GET", ctx.jwtAuth->jwt().aud());
            if (!res) {
                // token does not exist
                auto s = ctx.jwtAuth->authenticate(
                        "Attempt to access a protected resource with an invalid token");
                resp.end(s);
                return;
            }

            auto token = static_cast<String>(res);
            if (token != ctx.jwtAuth->token()) {
                // token is bad or might have been revoked
                auto s = ctx.jwtAuth->authenticate(
                        "Attempt to access a protected resource with an invalid token");
                resp.end(s);
                return;
            }
        }
    }

    void JwtSession::after(Request& req, Response&, Context& ctx)
    {
        // clear the context
        ctx.jwtSession = nullptr;
        ctx.jwtAuth = nullptr;
        ctx.redisContext = nullptr;
    }

}
