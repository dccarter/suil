//
// Created by Mpho Mbotho on 2020-12-16.
//

#ifndef SUIL_HTTP_SERVER_ROUTING_HPP
#define SUIL_HTTP_SERVER_ROUTING_HPP

#include <suil/http/route.scc.hpp>
#include <suil/http/server/crow.hpp>

namespace suil::http::server {

    class RouterParams {
    public:
        RouterParams(unsigned first, crow::detail::routing_params&& params);
        MOVE_CTOR(RouterParams) noexcept;
        MOVE_ASSIGN(RouterParams) noexcept;

        unsigned first{0};
        crow::detail::routing_params second;

    private:
        DISABLE_COPY(RouterParams);
    };

    struct RequestParams {
        RequestParams() = default;
        uint32                 index{0};
        crow::detail::routing_params decoded{};
        RouteAttributes*       attrs{nullptr};
        uint32                 methods{0};
        uint32                 routeId{0};
        void clear();

    private:
        DISABLE_COPY(RequestParams);
        DISABLE_MOVE(RequestParams);
    };
}
#endif //SUIL_HTTP_SERVER_ROUTING_HPP
