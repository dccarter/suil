//
// Created by Mpho Mbotho on 2020-12-19.
//

#include "suil/http/server/sysattrs.hpp"
#include "suil/http/server/response.hpp"
#include "suil/http/server/request.hpp"

namespace suil::http::server {

    void SystemAttrs::before(Request& req, Response&, Context&)
    {
        const auto& attrs = req.attrs();
        if (cookies || attrs.ParseCookies) {
            // parsing cookies has been requested by route
            req.parseCookies();
        }

        if (attrs.ParseForm) {
            // parsing request form has been requested by route
            req.parseForm();
        }
    }

    void SystemAttrs::after(Request& req, Response& resp, Context&)
    {
        const auto& attrs = req.attrs();
        if (attrs.ReplyType and resp.contentType().empty()) {
            // Content-Type not set, override with route preferred
            resp.setContentType(attrs.ReplyType.peek());
        }
    }
}