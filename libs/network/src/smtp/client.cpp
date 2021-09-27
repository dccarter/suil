//
// Created by Mpho Mbotho on 2020-11-22.
//


#include "suil/net/ssl.hpp"
#include "suil/net/tcp.hpp"
#include "suil/net/smtp/client.hpp"

#include <suil/base/base64.hpp>

namespace suil::net::smtp {

    constexpr const char* CRLF{"\r\n"};
    constexpr const char* CRLFCRLF{"\r\n\n"};

    void Client::setupInit(String server, int port)
    {
        if (sock != nullptr and sock->isOpen()) {
            endSession();
            sock = nullptr;
        }
        server = std::move(server);
        port = port;
    }

    bool Client::createAdaptor(bool forceSsl)
    {
        constexpr int deprecatedSecureSMTP{465};
        constexpr int standardSecureSMTP{587};

        bool needsTls = forceSsl or (port == standardSecureSMTP) or (port == deprecatedSecureSMTP);
        try {
            if (needsTls) {
                idebug("Creating SSL socket for email server {%s:%d}", server(), port);
                sock = std::make_unique<SslSock>();
            } else {
                idebug("Creating socket for email server {%s:%d}", server(), port);
                sock = std::make_unique<TcpSock>();
            }
            auto addr = ipremote(server(), port, 0, -1);
            if (errno) {
                idebug("resolving server address {%s:%d} failed: %s", server(), port, errno_s);
                return false;
            }
            if (!sock->connect(addr, config.sendTimeout)) {
                return false;
            }
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            ierror("creating adaptor failed: %s", ex.what());
            return false;
        }

        return true;
    }

    void Client::endSession()
    {
        if (sock and adaptor().isOpen()) {
            if (sendLine(5000, "QUIT")) {
                // QUIT command sent, read whatever the server send
                char buffer[32];
                size_t len{sizeof(buffer)};
                adaptor().read(buffer, len, 5000);
            }
            adaptor().close();
            sock = nullptr;
        }
    }

    bool Client::reset()
    {
        if (sock and adaptor().isOpen()) {
            if (!sendLine(config.sendTimeout, "RSET")) {
                iwarn("Sending RSET command failed: %s", errno_s);
                return false;
            }

            auto resp = getResponse(config.receiveTimeout);
            if (resp.code() != 250) {
                iwarn("Server rejected RSET command: (%d) %s", resp.code(), getError(resp));
                return false;
            }

            return true;
        }

        return false;
    }

    void Client::send(const Email& msg, const EmailAddress& from)
    {
        if (sock == nullptr or !adaptor().isOpen()) {
            throw SmtpClientError("Cannot send an email before logging in");
        }

        Response resp;
        if (!reset()) {
            throw SmtpClientError("Reseting connection failed");
        }

        if (!sendLine(config.sendTimeout, "MAIL FROM: <", from.addr, ">")) {
            throw SmtpClientError("Sending <MAIL FROM> failed: ", errno_s);
        }

        if ((resp = getResponse(config.mailFromTimeout)).code() != 250) {
            throw SmtpClientError("<MAIL FROM> error: (", resp.code(), ") ", getError(resp));
        }

        // send message head
        sendHead(msg);

        if (!sendLine(config.sendTimeout, "DATA")) {
            throw SmtpClientError("Sending <DATA> failed: ", errno_s);
        }

        if ((resp = getResponse(config.dataInitTimeout)).code() != 354) {
            throw SmtpClientError("Server rejected <DATA> command: (", resp.code(), ") ", getError(resp));
        }

        auto headers = msg.head(from);
        if (adaptor().send(headers.data(), headers.size(), config.sendTimeout) != headers.size()) {
            throw SmtpClientError("Sending email headers failed: ", errno_s);
        }

        if (!msg.attached.empty()) {
            // attached email, need to put body encoding
            auto status = sendParts(config.sendTimeout,
                     "--", msg.boundary, CRLF,
                    "Content-Type: ", msg.bodyType, ";charset=utf8", CRLF,
                    "Content-Encoding: 8bit", CRLFCRLF);
            if (!status) {
                throw SmtpClientError("Sending email body multipart header failed: ", errno_s);
            }
        }

        if (!msg.mBody.empty()) {
            // send email body
            if (adaptor().send(msg.mBody.data(), msg.mBody.size(), config.sendTimeout) != msg.mBody.size()) {
                throw SmtpClientError("Sending email body failed: ", errno_s);
            }
        }

        if (!msg.attached.empty()) {
            // send attachment's if any
            if (!sendPart(CRLFCRLF, config.sendTimeout)) {
                throw SmtpClientError("Sending attachments separator failed: ", errno_s);
            }

            for (const auto& it: msg.attached) {
                auto status = sendParts(config.sendTimeout,
                            "--", msg.boundary, CRLF,
                            "Content-Type: ", it.mime, "; name=\"", it.filename(), "\"", CRLF,
                            "Content-Disposition: attachment; filename=\"", it.filename(), "\"", CRLF,
                            "Content-Transfer-Encoding: 8bit", CRLFCRLF);
                if (!status) {
                    throw SmtpClientError("Sending attachment '", it.filename(), "' header failed: ", errno_s);
                }

                // send attachment
                it(adaptor(), msg.boundary, config.sendTimeout);
                if (!sendPart(CRLFCRLF, config.sendTimeout)) {
                    throw SmtpClientError("Sending attachments separator failed: ", errno_s);
                }
            }

            if (!sendParts(config.sendTimeout, "--", msg.boundary, CRLF)) {
                throw SmtpClientError("Sending end of attachments separator failed: ", errno_s);
            }
        }

        // send end of data token
        if (!sendLine(config.sendTimeout, CRLF, ".")) {
            throw SmtpClientError("Sending end of <DATA> token failed: %s", errno_s);
        }

        if ((resp = getResponse(config.dataTermTimeout)).code() != 250) {
            throw SmtpClientError("Server rejected email: (", resp.code(), ") ", getError(resp));
        }
    }

    bool Client::initConnection(const String& domain)
    {
        Response resp;
        // get server line
        if ((resp = getResponse(config.greetingTimeout)).code() != 220) {
            iwarn("SMTP connect (%s:%p) error: (%d) %s", server(), port, resp.code(), getError(resp));
            return false;
        }

        {
            // send EHLO first
            if (!sendLine(config.sendTimeout, "EHLO ", domain)) {
                throw SmtpClientError("Send EHLO <domain> failed: %s", errno_s);
            }

            if ((resp = getResponse(config.greetingTimeout)).code() == 250) {
                // populate extensions and return true
                return populateExtensions(resp.lines());
            }

            if ((resp.code() != 502)) {
                iwarn("Server rejected EHLO <domain>: (%d) %s", resp.code(), getError(resp));
                return false;
            }
        }

        {
            // assume server does not support EHLO, send HELO
            if (!sendLine(config.sendTimeout, "HELO ", domain)) {
                throw SmtpClientError("Send HELO <domain> failed: %s", errno_s);
                return false;
            }

            if ((resp = getResponse(config.greetingTimeout)).code() != 250) {
                // populate extensions and return true
                iwarn("Server rejected HELO <domain>: (%d) %s", resp.code(), getError(resp));
                return false;
            }
        }

        return true;
    }

    bool Client::doLogin(
            AuthMechanism mechanism,
            const String& username,
            const String& passwd,
            const String& domain,
            bool forceSsl)
    {
        if (!createAdaptor(forceSsl)) {
            return false;
        }


        bool status{true};
        // if at some point we failed, send QUIT command
        defer({
            if (!status) {
                // quit and destroy session
                endSession();
            }
        });

        if (!(status = initConnection(domain))) {
            // initializing connection failed
            return status;
        }

        auto authKeyword  = toString(mechanism);
        if (AUTH.find(mechanism) == AUTH.end()) {
            ierror("Authentication mechanism '%s' not supported by server", authKeyword());
            return status;
        }

        switch (mechanism) {
            case AuthMechanism::Plain:
                return (status = authPlain(username, passwd));
            case AuthMechanism::Login:
                return (status = authLogin(username, passwd));
            case AuthMechanism::MD5:
                return (status = authMD5(username, passwd));
            case AuthMechanism::DigestMD5:
                return (status = authDigestMD5(username, passwd));
            case AuthMechanism::CramMD5:
                return (status = authCramMD5(username, passwd));
            default:
                ierror("Unsupported username/password mechanism %d:%s", int(mechanism), authKeyword());
                return (status = false);
        }
    }

    bool Client::authLogin(const String& username, const String& password)
    {
        auto b64Username = Base64::encode(username);
        auto b64Passwd = Base64::encode(password);

        Response resp;
        if (!sendLine(config.sendTimeout, "AUTH LOGIN")) {
            throw SmtpClientError("Sending AUTH LOGIN <username> failed: ", errno_s);
        }

        if ((resp = getResponse(config.receiveTimeout)).code() != 334) {
            iwarn("Username rejected by server: (%d) %s", resp.code(), getError(resp));
            return false;
        }

        if (!sendLine(config.sendTimeout, b64Username)) {
            throw SmtpClientError("Sending AUTH LOGIN <username> failed: ", errno_s);
        }

        if ((resp = getResponse(config.receiveTimeout)).code() != 334) {
            iwarn("Password rejected by server: (%d) %s", resp.code(), getError(resp));
            return false;
        }

        if (!sendLine(config.sendTimeout, b64Passwd)) {
            throw SmtpClientError("Sending AUTH LOGIN <passwd> failed: ", errno_s);
        }

        if ((resp = getResponse(config.receiveTimeout)).code() != 235) {
            iwarn("Password rejected by server: (%d) %s", resp.code(), getError(resp));
            return false;
        }

        return true;
    }

    bool Client::authPlain(const String& username, const String& password)
    {
        char tmp[username.size()+password.size()+4];
        size_t index{0};
        tmp[index++] = '\0';
        memcpy(&tmp[index], username.data(), username.size());
        index += username.size();
        tmp[index++] = '\0';
        memcpy(&tmp[index], password.data(), password.size());
        index += password.size();

        auto authString = Base64::encode(reinterpret_cast<const uint8_t *>(tmp), index);
        Response resp;
        if (!sendLine(config.sendTimeout, "AUTH PLAIN ", authString)) {
            throw SmtpClientError("Sending AUTH PAIN <string> failed: ", errno_s);
        }

        if ((resp = getResponse(config.receiveTimeout)).code() != 235) {
            iwarn("Username/Password rejected by server: (%d) %s", resp.code(), getError(resp));
            return false;
        }

        return true;
    }

    void Client::sendAddresses(const std::vector<EmailAddress>& addrs)
    {
        Response resp;
        for (const auto& addr: addrs) {
            if (!sendLine(config.sendTimeout, "RCPT TO: <", addr.addr, ">")) {
                throw SmtpClientError("Sending RCPT TO: <email> failed: ", errno_s);
            }

            if ((resp = getResponse(config.rcptToTimeout)).code() != 250) {
                throw SmtpClientError(
                        "Server rejected RCP TO: <email>: (", resp.code(), ") ",
                        getError(resp));
            }
        }
    }

    void Client::sendHead(const Email& msg)
    {
        sendAddresses(msg.recipients);
        sendAddresses(msg.ccs);
        sendAddresses(msg.bccs);
    }

    Response Client::getResponse(const Deadline& dd)
    {
        constexpr size_t minSizeSent{4}; // XXX SP <Data>
        char ob[512];
        int code{0};
        Response resp;
        do {
            ob[3] = '\0';
            auto len{sizeof(ob)-1};
            if (!sock->receiveUntil(ob, len, CRLF, 2, dd)) {
                throw SmtpClientError("receiving response failed: ", errno_s);
            }
            size_t tmp{1};
            sock->receiveUntil(&ob[len], tmp, "\n", 1, dd);
            ob[--len] = '\0';
            if (len < minSizeSent) {
                throw SmtpClientError("Invalid output received from sever: ", ob);
            }
            auto data = (len < 4)? String{""} : String{&ob[4]}.dup();
            if (code == 0) {
                // parse response code
                char tmp[] = {ob[0], ob[1], ob[2], '\0'};
                suil::cast(String{tmp}, code);
                resp = Response::create(code, std::move(data));
            }
            else {
                resp.push(std::move(data));
            }
        } while (ob[3] == '-');

        return std::move(resp);
    }

    bool Client::sendFlush(const Deadline& dd)
    {
        if (adaptor().send(CRLF, 2, dd) == 2) {
            return adaptor().flush(dd);
        }
        return false;
    }

    bool Client::sendPart(const String& part, const Deadline& dd)
    {
        return adaptor().send(part.data(), part.size(), dd) == part.size();
    }

    const char* Client::getError(const Response& resp) const
    {
        if (resp.lines().empty()) {
            return getError(resp.code());
        }
        return resp.lines().front()();
    }

    const char* Client::getError(int code) const
    {
        switch (code) {
            case 252:
                return "Cannot verify user, but will accept message and attempt delivery";
            case 421:
                return "Service not available, closing transmission channel";
            case 450:
                return "Requested mail action not taken: mailbox unavailable";
            case 451:
                return "Requested action aborted: local error in processing";
            case 452:
                return "Requested action not taken: insufficient system storage";
            case 500:
                return "Syntax error, command unrecognised";
            case 501:
                return "Syntax error in parameters or arguments";
            case 502:
                return "Command not implemented";
            case 503:
                return "Bad sequence of commands";
            case 504:
                return "Command parameter not implemented";
            case 521:
                return "Domain does not accept mail (see rfc1846)";
            case 530:
                return "Access denied";
            case 550:
                return "Requested action not taken: mailbox unavailable";
            case 552:
                return "Requested mail action aborted: exceeded storage allocation";
            case 553:
                return "Requested action not taken: mailbox name not allowed";
            case 554:
                return "Transaction failed";
            default:
                if (code < 0) {
                    return strerror(-code);
                }
                return "Unknown/Unhandled error code";
        }
    }

    bool Client::populateExtensions(const std::vector<String>& resp)
    {

        for (int i = 1; i < resp.size(); i++) {
            auto& ext = resp[i];
            if (ext.size() < 4) {
                EXT.push_back(ext.dup());
                continue;
            }

#define code(a,b,c,d) ( (int(a)<<24) | (int(b)<<16) | (int(c)<<8) | int(d) )
            auto id = code(
                    std::toupper(ext[0]),
                    std::toupper(ext[1]),
                    std::toupper(ext[2]),
                    std::toupper(ext[3]));

            switch (id) {
                case code('S', 'I', 'Z', 'E'): {
                    if (ext.size() < 6) {
                        break;
                    }
                    // get the maximum size supported by the server
                    suil::cast(ext.substr(5), SIZE);
                    break;
                }
                case code('A', 'U', 'T', 'H'): {
                    const String tmp = ext.substr(5);
                    auto parts = tmp.parts(" ");
                    for (const auto& part: parts) {
                        auto mechanism = parseAuthMechanism(part);
                        if (mechanism != AuthMechanism::Unknown) {
                            AUTH.insert(mechanism);
                        }
                    }
                    break;
                }
                default:
                    EXT.push_back(ext.dup());
                    break;
            }
#undef code
        }

        return true;
    }

    Socket& Client::adaptor()
    {
        if (sock == nullptr) {
            throw SmtpClientError("Currently not connected to server");
        }
        return *sock;
    }

    SmtpOutbox::Composed::Composed(Email::Ptr email, int64_t timeout)
        : mEmail{std::move(email)},
          mTimeout{timeout}
    {}

    Email& SmtpOutbox::Composed::email()
    {
        if (mEmail == nullptr) {
            throw SmtpClientError("Composed email is invalid");
        }
        return *mEmail;
    }

    int64_t SmtpOutbox::Composed::timeout()
    {
        return mTimeout;
    }

    mill::Event& SmtpOutbox::Composed::sync()
    {
        return evSync;
    }

    SmtpOutbox::SmtpOutbox(Email::Address sender)
        : sender{std::move(sender)}
    {}

    SmtpOutbox::SmtpOutbox(String server, int port, Email::Address sender, std::int64_t timeout)
        : client{std::move(server), port},
          sender{std::move(sender)},
          sendTimeout{timeout}
    {}

    Email::Ptr SmtpOutbox::draft(const String& to, const String& subject)
    {
        return std::make_shared<Email>(to, subject);
    }

    String SmtpOutbox::send(Email::Ptr email, int64_t timeout)
    {
        auto outgoing = std::make_shared<Composed>(std::move(email), timeout);
        {
            mill::Lock lk{_mutex};
            sendQ.push_back(outgoing);
        }
        bool expected{false};
        if (Ego._sending.compare_exchange_weak(expected, true)) {
            go(sendOutbox(Ego));
        }

        if (timeout > 0) {
            char *msg{nullptr};
            outgoing->isCancelled = false;
            if (outgoing->sync().wait(timeout)) {
                // Composed email will be deleted by sending coroutine
                return std::move(outgoing->failureMsg);
            }
            else {
                outgoing->isCancelled = true;
                return String{"Sending email message timed out"};
            }
        }
        else {
            // this is a fire and forget
            return {};
        }
    }

    void SmtpOutbox::sendOutbox(SmtpOutbox& Self)
    {
        while (not Self.isQueueEmpty()) {
            auto outgoing = Self.popNextQueued();
            if (outgoing->isCancelled) {
                ltrace(&Self, "Outgoing message cancelled, no need to send");
                continue;
            }

            try {
                Self.client.send(outgoing->email(), Self.sender);
            }
            catch (...) {
                auto ex = Exception::fromCurrent();
                ldebug(&Self, "SmtpOutbox sending composed email failed: %s", ex.what());
                if (!outgoing->isCancelled) {
                    outgoing->failureMsg = String{ex.what()}.dup();
                }
            }

            // inform sender that message has been sent, if sender was waiting
            outgoing->evSync.notify();
            yield();
        }

        Self._sending = false;
    }

    bool SmtpOutbox::isQueueEmpty()
    {
        mill::Lock lk{_mutex};
        return quiting or sendQ.empty();
    }

    SmtpOutbox::Composed::Ptr SmtpOutbox::popNextQueued()
    {
        mill::Lock lk{_mutex};
        if (sendQ.empty()) {
            return nullptr;
        }
        auto ptr = std::move(sendQ.front());
        sendQ.pop_front();
        return ptr;
    }

}