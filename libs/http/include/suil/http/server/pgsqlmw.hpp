//
// Created by Mpho Mbotho on 2020-12-20.
//

#ifndef SUIL_HTTP_SERVER_PGSQLMW_HPP
#define SUIL_HTTP_SERVER_PGSQLMW_HPP

#include <suil/http/server/request.hpp>
#include <suil/http/server/response.hpp>

#include <suil/db/pgsql.hpp>

namespace suil::http::server {

    class PgSqlMiddleware {
    public:
        struct Context {
            inline db::PgSqlConnection& conn() {
                if (pgSqlMiddleware == nullptr) {
                    throw UnsupportedOperation("PgSqlMiddleware context can only be used by routes, "
                                               "or middlewares that follows");
                }
                return pgSqlMiddleware->conn();
            }
        private:
            friend class PgSqlMiddleware;
            PgSqlMiddleware* pgSqlMiddleware{nullptr};
        };

        PgSqlMiddleware() = default;

        void before(Request&, Response&, Context& ctx);
        void after(Request&, Response&, Context& ctx);


        template <typename Opts>
        void configure(Opts&& opts, const String& conn) {
            Ego._db.configure(std::forward<Opts>(opts), conn);
        }

        template <typename... Opts>
        void setup(const String& connStr, Opts&&... opts) {
            auto options = iod::D(std::forward<Opts>(opts)...);
            configure(options, connStr);
        }

        inline db::PgSqlConnection& conn(bool cached = true) {
            return Ego._db.connection(cached);
        }

    private:
        db::PgSqlDb _db;
    };
}
#endif //SUIL_HTTP_SERVER_PGSQLMW_HPP
