//
// Created by Mpho Mbotho on 2020-12-19.
//

#ifndef SUIL_HTTP_SERVER_INITIALIZER_HPP
#define SUIL_HTTP_SERVER_INITIALIZER_HPP

#include "suil/http/server/response.hpp"
#include "suil/http/server/request.hpp"
#include "suil/http/server/endpoint.hpp"

#include <suil/base/string.hpp>

namespace suil::http::server {

    class Initializer final {
    public:
        using Handler = std::function<bool(const Request&, Response&)>;

        struct Context {
        };

        void before(Request& req, Response& resp, Context& ctx);
        void after(Request& req,  Response& resp, Context& ctx);

        template <typename Ep>
        void setup(Ep& ep, Handler handler, bool form = true) {
            if (handler == nullptr) {
                Ego._blocked = false;
                return;
            }

            if (Ego._handler == nullptr) {
                Ego._initRoute =
                ep("/app-init")
                ("GET"_method, "POST"_method, "OPTIONS"_method)
                .attrs(opt(Authorize, Auth{false}), opt(ParseForm, form))
                ([&](const Request& req, Response& resp) {
                    init(req, resp);
                });

                Ego._blocked =  true;
                Ego._handler = handler;
            }
        }

    private:
        void init(const Request& req, Response& resp);

        bool     _blocked{false};
        bool     _unblock{false};
        uint32   _initRoute{0};
        Handler  _handler{nullptr};
    };
}
#endif //SUIL_HTTP_SERVER_INITIALIZER_HPP
