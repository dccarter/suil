//
// Created by Mpho Mbotho on 2020-12-19.
//

#include "suil/http/server/cors.hpp"
#include "suil/http/server/response.hpp"
#include "suil/http/server/request.hpp"

namespace suil::http::server {

    void Cors::before(Request& req, Response& resp, Context&)
    {
        if (req.getMethod() == Method::Options) {
            if (Ego._allowOrigin) {
                resp.header("Access-Control-Allow-Origin", Ego._allowOrigin.peek());
            }

            if (Ego._allowHeaders) {
                resp.header("Access-Control-Allow-Headers", Ego._allowHeaders.peek());
            }
            resp.end();
        }
    }

    void Cors::after(Request& req, Response& resp, Context&)
    {
        if (req.getMethod() == Method::Options) {
            /* Requesting options */
            auto& reqMethod = req.header("Access-Control-Request-Method");
            if (reqMethod) {
                auto corsMethod = http::fromString(reqMethod);
                if ((req.params().methods & (1 << uint32(corsMethod)))) {
                    // only if route supports requested method
                    resp.header("Access-Control-Allow-Methods", reqMethod.peek());
                }
            }
        }
    }
}