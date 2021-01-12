//
// Created by Mpho Mbotho on 2020-11-22.
//

#include "suil/net/smtp/protocol.hpp"
#include "suil/net/smtp/context.hpp"
#include "suil/net/smtp/common.hpp"

namespace suil::net::smtp {

    constexpr const char* CRLF{"\r\n"};

    ServerProtocol::ServerProtocol(Socket& sock, ServerContext& context)
        : sock{sock},
          context{context}
    {}

    void ServerProtocol::startSession() {
        if (context.isReady()) {
            // server not ready
            sendLine(5000, "421 Service not available", CRLF);
            return;
        }

        auto resp = context.onConnect(session, sock.id());
        sendResponse(resp);

        State state{getNextState()};
        while (state != State::Abort) {
            switch (state) {
                case State::ExpectCommand:
                    state = receiveCommand();
                    break;
                case State::ExpectLine:
                    state = receiveLine();
                    break;
                case State::ExpectData:
                    state = receiveData();
                    break;
                case State::KeepAliveExceeded: {
                    sendResponse(Response::abort(0, ""));
                    state = State::Abort;
                    break;
                }
                default:
                    iwarn("Unknown SmtpProtocol State %d", int(state));
                    state = State::Abort;
                    break;
            }
        }
    }

    ServerProtocol::State ServerProtocol::receiveCommand()
    {
        char qb[1024];
        size_t len{sizeof(qb)-2};
        if (!sock.receiveUntil(qb, len, CRLF, 2, context.config().waitTimeout)) {
            if (errno != 0)
                idebug("receiving command line failed: %s", errno_s);
            return (errno == ETIMEDOUT)? State::KeepAliveExceeded : State::Abort;
        }
        // drop the CRLF characters
        size_t one{1};
        sock.receiveUntil(&qb[len], one, CRLF, 2, context.config().waitTimeout);
        qb[--len] = '\0';
        String line{qb, len, false};

        auto [cmd, arg] = parseCommand(line);

        auto resp = context.onCommand(session, cmd, arg);
        sendResponse(resp);
        if (resp.isAbort()) {
            // connection aborted by server
            return State::Abort;
        }

        return getNextState();
    }

    ServerProtocol::State ServerProtocol::receiveLine()
    {
        char qb[1024];
        size_t len{sizeof(qb)-2};
        if (!sock.receiveUntil(qb, len, CRLF, 2, context.config().waitTimeout)) {
            idebug("receiving data line failed: %s", errno_s);
            return (errno == ETIMEDOUT)? State::KeepAliveExceeded : State::Abort;
        }
        size_t one{1};
        sock.receiveUntil(&qb[len], one, CRLF, 2, context.config().waitTimeout);
        qb[--len] = '\0';
        // drop the CRLF characters
        String str{qb, len, false};
        auto resp = context.onLine(session, str);
        sendResponse(resp);
        if (resp.isAbort()) {
            return State::Abort;
        }

        return getNextState();
    }

    ServerProtocol::State ServerProtocol::receiveData()
    {
        char qb[8192], term[6];
        auto state = State::ExpectData;
        size_t totalRead{0};
        do {
            size_t len{sizeof(qb)};
            if (!sock.read(qb, len, context.config().waitTimeout)) {
                idebug("reading data from client failed: %s", errno_s);
                return (errno == ETIMEDOUT)? State::KeepAliveExceeded : State::Abort;
            }
            totalRead += len;
            auto resp = context.onData(session, Data{qb, len, false});
            if (resp) {
                // All data received, send response
                sendResponse(*resp);
            }
        } while (state == getNextState());

        return getNextState();
    }

    std::pair<Command, String> ServerProtocol::parseCommand(const String& line)
    {
        constexpr std::size_t MinCommandSize{4};
        if (line.size() < MinCommandSize) {
            // all SMTP command are at least 4 bytes
            return {Command::Unrecognized, ""};
        }

        Command cmd{Command::Unrecognized};

        auto tmp = line.substr(0, 4);
        tmp.toupper();
        auto c0 = code_r(&tmp[0], true);
        auto getArgument = [&](std::size_t offset) {
            if (offset < line.size()) {
                // copy the string out
                auto str = line.substr(offset, line.size()-offset);
                return str;
            }
            else {
                return String{};
            }
        };
        switch (c0) {
            case code("HELO"): {
                return {Command::Hello, getArgument(5)};
            }
            case code("EHLO"): {
                return {Command::EHello, getArgument(5)};
            }
            case code("DATA"): {
                return {Command::Data, String{}};
            }
            case code("RCPT"): {
                if ((line.size() < 7) or (strncasecmp(&line[4], " TO", 3) != 0)) {
                    break;
                }
                else {
                    return {Command::RcptTo, getArgument(7)};
                }
            }
            case code("MAIL"): {
                if (line.size() < 9 or (strncasecmp(&line[4], " FROM", 5) != 0)) {
                    break;
                }
                else {
                    return {Command::MailFrom, getArgument(9)};
                }
            }
            case code("NOOP"): {
                return {Command::Noop, String{}};
            }
            case code("HELP"): {
                return {Command::Help, getArgument(5)};
            }
            case code("VRFY"): {
                return {Command::Verify, getArgument(5)};
            }
            case code("EXPN"): {
                return {Command::Expn, getArgument(5)};
            }
            case code("RSET"): {
                return {Command::Reset, getArgument(5)};
            }
            case code("QUIT"): {
                return {Command::Quit, getArgument(5)};
            }
            case code("STAR"): {
                if (line.size() < 8 or (strncasecmp(&line[4], "TTLS", 4) != 0)) {
                    break;
                }
                return {Command::StartTls, getArgument(9)};
            }
            case code("AUTH"): {
                return {Command::Auth, getArgument(5)};
            }
            case code("ATRN"): {
                return {Command::ATrn, getArgument(5)};
            }
            case code("BDAT"): {
                return {Command::BData, getArgument(5)};
            }
            case code("ETRN"): {
                return {Command::ETrn, getArgument(5)};
            }
            default:
                break;
        }
#undef code
        idebug("Unrecognized command: %.*s", std::min(size_t(10), line.size()), line());
        return {Command::Unrecognized, String{}};
    }

    void ServerProtocol::sendResponse(const Response& resp)
    {
        char buf[4];
        snprintf(buf, 4, "%03d", resp.code());
        Deadline dd{context.config().sendTimeout};
        for (const auto& line: resp.lines()) {
            if (&line == &resp.lines().back()) {
                break;
            }
            if (!sendParts(dd, buf, "-", line, CRLF)) {
                throw SmtpProtocolError("Sending response '", buf, "-", line, "' failed: ", errno_s);
            }
        }
        if (!sendLine(dd, buf, " ", resp.lines().back())) {
            throw SmtpProtocolError(
                    "Sending response '", buf, " ", resp.lines().back(), "' failed: ", errno_s);
        }
    }

    ServerProtocol::State ServerProtocol::getNextState()
    {
        switch (session.getState()) {
            case Session::State::ExpectHello:
            case Session::State::ExpectAuth:
            case Session::State::ExpectMail:
            case Session::State::ExpectRecipient:
                return State::ExpectCommand;
            case Session::State::AuthExpectUsername:
            case Session::State::AuthExpectPassword:
                return State::ExpectLine;
            case Session::State::ExpectData:
                return State::ExpectData;
            case Session::State::Quit:
            default:
                return State::Abort;
        }
    }

    bool ServerProtocol::sendPart(const String& part, const Deadline& dd)
    {
        return sock.send(part.data(), part.size(), dd) == part.size();
    }

    bool ServerProtocol::sendFlush(const Deadline& dd)
    {
        if (sock.send(CRLF, 2, dd) == 2) {
            return sock.flush(dd);
        }
        return false;
    }
}
