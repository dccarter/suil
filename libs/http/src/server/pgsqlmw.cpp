//
// Created by Mpho Mbotho on 2020-12-20.
//

#include "suil/http/server/pgsqlmw.hpp"

namespace suil::http::server {

    void PgSqlMiddleware::before(Request&, Response&, Context& ctx)
    {
        ctx.pgSqlMiddleware = this;
    }

    void PgSqlMiddleware::after(Request&, Response&, Context& ctx)
    {
        ctx.pgSqlMiddleware = nullptr;
    }

}