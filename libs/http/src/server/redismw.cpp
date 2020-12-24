//
// Created by Mpho Mbotho on 2020-12-20.
//

#include "suil/http/server/redismw.hpp"

namespace suil::http::server {

    db::RedisClient & RedisMiddleware::Context::conn(int db)
    {
        if (redisMiddleware == nullptr) {
            throw UnsupportedOperation(""
                                       "RedisMiddlewareContext should only accessed by routes"
                                       "or middlewares that come after it");
        }
        return redisMiddleware->conn(db);
    }

    void RedisMiddleware::before(Request&, Response&, Context& ctx) {
        // just initialize the content
        ctx.redisMiddleware = this;
    }

    void RedisMiddleware::after(Request&, Response&, Context& ctx) {
        ctx.redisMiddleware = nullptr;
    }
}