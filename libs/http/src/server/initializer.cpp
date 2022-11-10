//
// Created by Mpho Mbotho on 2020-12-19.
//

#include "suil/http/server/initializer.hpp"

namespace suil::http::server {

    void Initializer::before(Request& req, Response& resp, Context& ctx)
    {
        if (!Ego._blocked) {
            // allow normal processing
            return;
        }

        if (req.routeId() != Ego._initRoute) {
            // reject all request except to the init route
            resp.end(http::Status::ServiceUnavailable);
        }
    }

    void Initializer::after(Request& req, Response& resp, Context& ctx)
    {
        if (Ego._blocked) {
            if (ctx._unblock) {
                Ego._blocked = false;
                req.enabled(false);
                ipc::broadcast(INITIALIZER_ENABLE);
            }
        }
    }

    void Initializer::enable(Router& router)
    {
        sdebug("Enabling all routes");   
        if (!Ego._blocked) {
            Ego._blocked = true;
            router | [&](BaseRule& rule) -> bool {
                if (Ego._initRoute == rule.id()) {
                    rule._attrs.Enabled = false;
                    return false;
                }
                return true;
            };
        }
    }
}