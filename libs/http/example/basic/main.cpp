//
// Created by Mpho Mbotho on 2020-12-18.
//

#include "suil/http/server/endpoint.hpp"
#include "suil/http/server/sysattrs.hpp"
#include "suil/http/server/admin.hpp"

namespace hs = suil::http::server;
namespace net  = suil::net;

int main(int argc, char *argv[])
{
    using Server = hs::Endpoint<>;

    net::ServerConfig sock = {
        .socketConfig = net::TcpSocketConfig{
            .bindAddr = { .name = "0.0.0.0", .port = 8000 }
        }
    };

    // Create an http endpoint
    Server ep("/",
              opt(serverConfig, std::move(sock)),
              opt(keepAliveTime, 5_min),
              opt(numberOfWorkers, 4));

    // Add an unsecure route used for login
    Route(ep, "/echo/1")
    ("GET"_method, "OPTIONS"_method)
    .attrs(opt(ReplyType, "text/plain"))
    ([&](const hs::Request& req, hs::Response& res) {
        // body will be cleared after response
        res.chunk({const_cast<char *>(req.body().data()), req.body().size(), 0});
    });

    Route(ep, "/echo/2")
    ("GET"_method, "OPTIONS"_method)
    .attrs(opt(ReplyType, "text/plain"))
    ([&](const hs::Request& req, hs::Response& res) {
        // body will be cleared after response
        res << req.body();
    });

    Route(ep, "/hello")
    ("GET"_method, "OPTIONS"_method)
    .attrs(opt(ReplyType, "text/plain"))
    ([&]() {
        // body will be cleared after response
        return suil::String{"Hello World"};
    });

    Route(ep, "/whoami/{string}")
    ("GET"_method, "OPTIONS"_method)
    .attrs(opt(ReplyType, "text/plain"))
    ([&](suil::String name) {
        // body will be cleared after response
        return suil::catstr("Hello ", name, " from ", mtid());
    });

    return ep.start();
}