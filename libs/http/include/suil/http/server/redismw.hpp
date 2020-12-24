//
// Created by Mpho Mbotho on 2020-12-20.
//

#ifndef SUIL_HTTP_SERVER_REDISMW_HPP
#define SUIL_HTTP_SERVER_REDISMW_HPP

#include <suil/http/server/request.hpp>
#include <suil/http/server/response.hpp>

#include <suil/db/redis.hpp>

namespace suil::http::server {

    class RedisMiddleware {
    public:
        struct Context {
            db::RedisClient& conn(int db = 0);

        private:
            friend class RedisMiddleware;
            RedisMiddleware *redisMiddleware{nullptr};
        };

        RedisMiddleware() = default;

        void before(Request& req, Response& resp, Context& ctx);

        void after(Request& req, Response&, Context& ctx);

        template <typename... Opts>
        inline void setup(const String& host, int port, Opts&&... opts) {
            Ego._db.setup(host, port, std::forward<Opts>(opts)...);
        }

        inline db::RedisClient& conn(int db = 0) {
            return Ego._db.connect(db);
        }

    private:
        DISABLE_COPY(RedisMiddleware);
        DISABLE_MOVE(RedisMiddleware);
        db::RedisDb _db{};
    };
}
#endif //SUIL_HTTP_SERVER_REDISMW_HPP
