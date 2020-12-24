//
// Created by Mpho Mbotho on 2020-11-23.
//

#ifndef SUILNETWORK_PROTOCOL_HPP
#define SUILNETWORK_PROTOCOL_HPP

#include "suil/net/smtp/command.hpp"
#include "suil/net/smtp/session.hpp"
#include "suil/net/smtp/response.hpp"
#include "suil/net/socket.hpp"

#include <suil/base/exception.hpp>
#include <suil/base/string.hpp>
#include <suil/base/logging.hpp>

namespace suil::net::smtp {

    class ServerContext;

    define_log_tag(SMTP_PROTO);

    DECLARE_EXCEPTION(SmtpProtocolError);

    class  ServerProtocol : LOGGER(SMTP_PROTO) {
    private:
        enum class State {
            Abort,
            KeepAliveExceeded,
            ExpectCommand,
            ExpectLine,
            ExpectData
        };

        bool sendFlush(const Deadline& dd);
        bool sendPart(const String& part, const Deadline& dd);
        State receiveCommand();
        State receiveLine();
        State receiveData();
        std::pair<Command,String> parseCommand(const String& line);
        void sendResponse(const Response& resp);

        template <typename ...Parts>
        bool sendParts(const Deadline& dd, const String& part, const Parts... parts) {
            if (!sendPart(part, dd)) {
                return false;
            }
            if constexpr (sizeof...(parts) > 0) {
                sendParts(dd, parts...);
            }
            return true;
        }

        template <typename ...Parts>
        bool sendLine(const Deadline& dd, const String& part, const Parts&... parts) {
            if (!sendParts(dd, part, parts...)) {
                return false;
            }
            return sendFlush(dd);
        }

        State getNextState();

    private:
        friend class ServerHandler;
        ServerProtocol(Socket& sock, std::shared_ptr<ServerContext> context);
        void startSession();
        Socket& sock;
        std::shared_ptr<ServerContext> context{nullptr};
        Session session;
    };
}

#endif //SUILNETWORK_PROTOCOL_HPP
