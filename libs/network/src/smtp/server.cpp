//
// Created by Mpho Mbotho on 2020-11-23.
//

#include "suil/net/smtp/server.hpp"
#include "suil/net/smtp/protocol.hpp"
#include "suil/net/smtp/context.hpp"

namespace suil::net::smtp {

    constexpr const char *CRLF{"\r\n"};
    constexpr const char *CRLFCRLF{"\r\n\n"};

    void ServerHandler::operator()(Socket& sock, ServerContext& context) {
        ServerProtocol proto(sock, context);
        try {
            proto.startSession();
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            idebug("unhandled exception %s", ex.what());
        }
    }

}