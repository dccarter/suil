//
// Created by Mpho Mbotho on 2020-11-22.
//

#ifndef SUILNETWORK_SMTP_SERVER_HPP
#define SUILNETWORK_SMTP_SERVER_HPP

#include <suil/base/logging.hpp>
#include "suil/net/server.hpp"

namespace suil::net::smtp {

    define_log_tag(SMTP_SERVER);
    class ServerContext;

    class ServerHandler : LOGGER(SMTP_SERVER) {
    public:
        void operator()(Socket& sock, ServerContext& context);
    };

    class Server : public net::Server<ServerHandler, ServerContext>, LOGGER(SMTP_SERVER) {
    public:
        using LOGGER(SMTP_SERVER)::log;
        using net::Server<ServerHandler, ServerContext>::Server;
    };
}
#endif //SUILNETWORK_SMTP_SERVER_HPP
