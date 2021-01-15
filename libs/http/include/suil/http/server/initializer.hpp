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
            inline void unblock() {
                _unblock = true;
            }
        private:
            friend class Initializer;
            bool     _unblock{false};
        };

        void before(Request& req, Response& resp, Context& ctx);
        void after(Request& req,  Response& resp, Context& ctx);

        template <typename Ep>
        DynamicRule& setup(Ep& ep)
        {
            auto& route = ep("/app-init");
            route("POST"_method, "OPTIONS"_method)
            .attrs(opt(Authorize, false));

            _initRoute = route.id();
            _blocked = true;

            return route;
        }

    private:
        void init(const Request& req, Response& resp);

        std::atomic_bool     _blocked{false};
        uint32   _initRoute{0};
    };
}
#endif //SUIL_HTTP_SERVER_INITIALIZER_HPP
