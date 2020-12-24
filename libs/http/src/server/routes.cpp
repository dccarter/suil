//
// Created by Mpho Mbotho on 2020-12-18.
//

#include <suil/http/server/routes.hpp>

namespace suil::http::server {

    RouterParams::RouterParams(unsigned int first, crow::detail::routing_params&& params)
        : first{first},
          second{std::move(params)}
    {}

    RouterParams::RouterParams(RouterParams&& other) noexcept
        : first{other.first},
          second{std::move(other.second)}
    {}

    RouterParams& RouterParams::operator=(RouterParams&& other) noexcept
    {
        Ego.first = other.first;
        Ego.second = std::move(other.second);
        return Ego;
    }

    void RequestParams::clear()
    {
        Ego.attrs = nullptr;
        Ego.decoded.clear();
    }
}