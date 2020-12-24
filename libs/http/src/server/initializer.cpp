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
            if (Ego._unblock) {
                Ego._blocked = false;
                req.enabled(false);
                Ego._unblock = false;
            }
        }
    }

    void Initializer::init(const Request& req, Response& resp)
    {
        if (Ego._handler != nullptr) {
            // invoke initialization handler
            Ego._unblock = Ego._handler(req, resp);
        }
    }
}