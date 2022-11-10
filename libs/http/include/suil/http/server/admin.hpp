//
// Created by Mpho Mbotho on 2020-12-19.
//

#ifndef SUIL_HTTP_SERVER_ADMIN_HPP
#define SUIL_HTTP_SERVER_ADMIN_HPP

#include "suil/http/server/response.hpp"
#include "suil/http/server/request.hpp"

#include <suil/base/sio.hpp>
#include <suil/base/string.hpp>

namespace suil::http::server {

    class EndpointAdmin {
    public:
        EndpointAdmin() = delete;
        DISABLE_COPY(EndpointAdmin);
        DISABLE_MOVE(EndpointAdmin);

        template <typename  E>
        static void setup(E& ep, const String& auth = "System-Admin") {
            ldebug(&ep, "initializing administration endpoint");
            // Use dynamic routes
            ep("/_admin/routes")
            ("GET"_method, "OPTIONS"_method)
            .attrs(opt(Authorize, Auth{auth.dup()}))
            ([&ep]() {
                std::vector<RouteSchema> schemas;
                ep.router() | [&schemas](const BaseRule& rule) {
                    if (rule.path().startsWith("/_admin")) {
                        // do not include admin routes
                        return true;
                    }

                    schemas.emplace_back(rule.schema());
                    return true;
                };

                return schemas;
            });

            ep("/_admin/route/{uint}")
            ("GET"_method, "OPTIONS"_method)
            .attrs(opt(Authorize, Auth{auth.dup()}))
            ([&ep](const Request&, Response& resp, uint32_t id) {
                auto status{http::NoContent};
                ep.router() | [&ep, &resp, &status, id](const BaseRule& rule) {
                    if (rule.path().startsWith("/_admin")) {
                        // do not include admin routes
                        return true;
                    }

                    if (rule.id() == id) {
                        RouteSchema schema{};
                        rule.schema(schema);
                        resp << schema;
                        status = Status::Ok;
                        return false;
                    }
                    return true;
                };

                resp.end(status);
            });

            ep("/_admin/route/{uint}/enable")
            ("POST"_method, "OPTIONS"_method)
            .attrs(opt(Authorize, Auth{auth.dup()}))
            ([&ep](uint32_t id) {
                return setRouteEnabled(ep, id, true);
            });

            ep("/_admin/route/{uint}/disable")
            ("POST"_method, "OPTIONS"_method)
            .attrs(opt(Authorize, Auth{auth.dup()}))
            ([&ep](uint32_t id) {
                return setRouteEnabled(ep, id, false);
            });

            ep("/_admin/endpoint/stats")
            ("GET"_method, "OPTIONS"_method)
            .attrs(opt(Authorize, Auth{auth.dup()}))
            ([&ep]{
                return ep.stats();
            });
        }

    private:
        template <typename E>
        static Status setRouteEnabled(E& ep, uint32_t id, bool en) {
            auto status{Status::NoContent};
            ep.router() | [&ep, &status, id, en](BaseRule& rule) {
                if (rule.path().startsWith("/_admin")) {
                    // do not include admin routes
                    return true;
                }

                if (rule.id() == id) {
                    rule._attrs.Enabled = en;
                    status = Status::Accepted;
                    ldebug(&ep, "disabling route " PRIs, _PRIs(rule.path()));
                    return false;
                }
                return true;
            };
            return status;
        }
    };

    template <typename ...Mws>
    void Admin(Endpoint<Mws...>& ep)
    {
        EndpointAdmin::setup(ep);
    }

    template <typename ...Mws>
    void Ping(Endpoint<Mws...>& ep, const char* route = "/ping")
    {
        ep(route)
        ("GET"_method)
        ([]{
            return Status::Ok;
        });
    }
}
#endif //SUIL_HTTP_SERVER_ADMIN_HPP
