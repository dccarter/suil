//
// Created by Mpho Mbotho on 2020-12-19.
//

#ifndef SUIL_HTTP_SERVER_INITIALIZER_HPP
#define SUIL_HTTP_SERVER_INITIALIZER_HPP

#include "suil/http/server/response.hpp"
#include "suil/http/server/request.hpp"
#include "suil/http/server/endpoint.hpp"

#include <suil/base/ipc.hpp>
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
        DynamicRule& setup(Ep& ep, bool blocked = false)
        {
            auto& route = ep("/app-init");
            route("POST"_method, "OPTIONS"_method)
            .attrs(opt(Authorize, false));

            _initRoute = route.id();
            _blocked = blocked;

            return route;
        }

    private:
        template <typename ...Mws>
        friend auto& Initialize(Endpoint<Mws...>& ep, bool);
        void enable(Router& router);

        std::atomic_bool     _blocked{false};
        uint32   _initRoute{0};
    };

    template <typename ...Mws>
    auto& Initialize(Endpoint<Mws...>& ep, bool blocked)
    {
        auto& initializer = ep.template middleware<Initializer>();
        ipc::registerHandler(INITIALIZER_ENABLE, 
        [&](uint8 /*unsued*/, uint8 * /*unused*/, size_t /*unused*/, bool /*unused*/) {
            initializer.enable(ep.router());
            return false;
        });
        return initializer.setup(ep, blocked);
    }
}
#endif //SUIL_HTTP_SERVER_INITIALIZER_HPP
