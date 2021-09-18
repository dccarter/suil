//
// Created by Mpho Mbotho on 2020-12-18.
//

#include <suil/http/common.hpp>
#include "suil/http/server/endpoint.hpp"
#include "suil/http/server/sysattrs.hpp"
#include "suil/http/server/cors.hpp"
#include "suil/http/server/admin.hpp"
#include "suil/http/server/initializer.hpp"
#include "suil/http/server/jwtauth.hpp"
#include "suil/http/server/jwtsession.hpp"
#include "suil/http/server/redismw.hpp"
#include "suil/http/server/fileserver.hpp"

namespace hs = suil::http::server;
namespace net  = suil::net;
using suil::http::Jwt;

struct User {
    suil::String passwd{};
    std::vector<suil::String> roles{};
};

const suil::UnorderedMap<User> users = {
        {"admin", {"admin123", {"System-Admin"}}},
        {"demo", {"example123", {"software"}}}
};

int main(int argc, char *argv[])
{
    using Server = hs::Endpoint<
                        hs::Initializer,        // Block all routes until application is initialized
                        hs::SystemAttrs,        // System level attributes
                        hs::Cors,               // CORS
                        hs::JwtAuthorization,   // Enable JWT authorization
                        hs::RedisMiddleware,    // Provides access to a redis database
                        hs::JwtSession          // Caches JWT's
                    >;

    net::ServerConfig sock = {
        .socketConfig = net::TcpSocketConfig{
            .bindAddr = { .name = "0.0.0.0", .port = 8000 }
        }
    };

    // Create an http endpoint
    Server ep("/api", opt(serverConfig, std::move(sock)) ,opt(keepAliveTime, 5_min));

    // Add endpoint administration routes
    hs::EndpointAdmin::setup(ep);

    // Configure hs::Initializer middleware
    ep.middleware<hs::Initializer>().setup(ep)
        ([&ep](const hs::Request& req, hs::Response& resp) {
            resp << "Server initialized by: " << req.query().get("admin");
            ep.context<hs::Initializer>(req).unblock();
        });

    // Configure hs::JwtAuthorization middleware
    ep.middleware<hs::JwtAuthorization>().setup(opt(key, "dzHzHvr"));

    // Configure hs::RedisMiddelware
    ep.middleware<hs::RedisMiddleware>().setup("redis", 6379);

    // attach a file server to the endpoint
    hs::FileServer fileServer(ep);

    // Configure JWT session
    auto& session = ep.middleware<hs::JwtSession>();
    session.setup(opt(refreshTokenKey, "cR4jxGGe8q4mdKN0f1rhkA=="));

    session.
        refresh(ep, opt(onTokenRefresh, "/refresh"))
        ([&](const hs::Request& req, hs::Response& resp) {
            auto& auth = ep.context<hs::JwtAuthorization>(req);

            auto& aud = auth.jwt().aud();
            auto it = users.find(aud);
            if (it == users.end()) {
                // Dead & Gone
                resp.end(suil::http::Forbidden);
                return;
            }

            // Just need to create a new token and return it the user
            auto& [name, user] = *it;
            Jwt jwt{};
            jwt.aud(name.peek());
            jwt.roles(user.roles);
            auth.authorize(std::move(jwt));
            resp << R"({"accessToken": ")" << auth.token() << R"("})";
        });

    session.
        refresh(ep, opt(onTokenRevoke, "/revoke"))
        ([]() {
            // nothing for us to do token will be revoked by middleware
            return suil::http::NoContent;
        });

    // Add an unsecure route used for login
    Route(ep, "/login")
    ("GET"_method, "OPTIONS"_method)
    .attrs(opt(ReplyType, "application/json"))
    ([&](const hs::Request& req, hs::Response& res) {
        const auto& user = req.query().get("username");
        const auto& passwd = req.query().get("passwd");

        auto it = users.find(user);
        if (it == users.end() or it->second.passwd != passwd) {
            // invalid credentials provided
            res << "Invalid credentials";
            res.end(suil::http::Forbidden);
            return;
        }

        auto& jwtSession = ep.context<hs::JwtSession>(req);
        jwtSession.revoke(user);
        Jwt jwt;
        jwt.aud(user.peek());
        jwt.roles(it->second.roles);
        if (jwtSession.authorize(std::move(jwt))) {
            res << R"({"accessToken": ")"
                << ep.context<hs::JwtAuthorization>(req).token()
                << R"(", "refreshToken": ")"
                << jwtSession.token()
                << R"("})";
        }
    });

    // Add a secure route
    // accepts PUT http methods
    // Only users authorized and belonging to software group are allowed
    Route(ep, "/modify/{string}/{int}")
    ("PUT"_method, "OPTIONS"_method)
    .attrs(opt(Authorize, hs::Auth{"software"_str}))
    ([&](const hs::Request& req, hs::Response& resp, suil::String key, int id) {
        auto& jwtAuth = ep.context<hs::JwtAuthorization>(req);
        scoped(conn, ep.context<hs::RedisMiddleware>(req).conn());
        conn.hset(jwtAuth.jwt().aud(), key, id);
    });

    return ep.start();
}