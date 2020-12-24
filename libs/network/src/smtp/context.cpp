//
// Created by Mpho Mbotho on 2020-11-23.
//

#include "suil/net/smtp/context.hpp"
#include "suil/net/smtp/session.hpp"
#include "suil/net/smtp/protocol.hpp"
#include "suil/net/smtp/common.hpp"

#include <suil/base/base64.hpp>

namespace suil::net::smtp {

    const String ServerContext::UsernameB64{Base64::encode(String{"Username:"})};
    const String ServerContext::PasswordB64{Base64::encode(String{"PasswordL"})};

#define _GUARDED(fn, ...) try { \
        fn (__VA_ARGS__) ;      \
    } catch(...) {            \
        auto ex = Exception::fromCurrent(); \
        iwarn(#fn "un-handled exception %s", ex.what()); \
    }

    template <typename Obj, typename ...Args>
    bool guarded(Response& ret, Obj& obj, Response (Obj::*func)(Args...), Args&&... args)
    {
        try {
            ret = (obj.*func)(std::forward<Args>(args)...);
            return true;
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            lwarn(&obj, "un-handle error in context: %s", ex.what());
            return false;
        }
    }

    class DefaultSessionData : public SessionData {
    public:
        DefaultSessionData(const String& id)
            : content{nullptr}
        {
            auto idx = id.find(':');
            if (idx > 0) {
                box = suil::catstr("/tmp/smail/", Base64::encode(id.substr(0, idx)));
            }
            else {
                box = suil::catstr("/tmp/smail/", Base64::encode(id));
            }
        }

        void open() {
            if (!fs::exists(box())) {
                // create directory
                fs::mkdir(box(), true);
            }
            if (content.valid()) {
                // close current file
                content.close();
            }

            auto path = suil::catstr(box, "/", id++);
            while (fs::exists(path())) path = suil::catstr(box, "/", id++);
            content = File(path(), O_WRONLY|O_CREAT|O_APPEND, 077);
        }

        void close() {
            // close current email
            if (content.valid()) {
                content.close();
            }
        }

        File& operator()() {
            if (!content.valid()) {
                open();
            }
            return content;
        }

        virtual ~DefaultSessionData() = default;
    private:
        String box;
        int id{0};
        File content;
    };

    const String& ServerContext::hostname()
    {
        static String defaultHostName{"suil-localhost"};
        return defaultHostName;
    }

    bool ServerContext::isReady()
    {
        return true;
    }

    const SmtpServerConfig& ServerContext::config()
    {
        return mConfig;
    }

    void ServerContext::addExtension(String extension)
    {
        if (mExtesions.find(extension) == mExtesions.end()) {
            mExtesions.insert(mExtesions.end(), extension);
        }
    }

    Response ServerContext::onConnect(Session& session, const String& id)
    {
        session.id = id.dup();
        session.setData<DefaultSessionData>(id.peek());
        addExtension("AUTH LOGIN PLAIN");
        return Response::create(220, suil::catstr(hostname(), " SMTP ", id));
    }

    Response ServerContext::onCommand(Session& session, Command cmd, const String& arg)
    {
        ApiReturn ret;

        switch (cmd) {
            case Command::Hello:
            case Command::EHello: {
                return  hello(session, arg, cmd == Command::EHello);
            }
            case Command::Auth: {
                return startAuth(session, arg);
            }
            case Command::Quit: {
                return quit(session, arg);
            }
            case Command::MailFrom: {
                return startEmail(session, arg);
            }
            case Command::RcptTo: {
                return addRecipient(session, arg);
            }
            case Command::Data:
            case Command::BData: {
                return startData(session, arg);
            }
            case Command::Help: {
                return onHelp(session, arg);
            }
            case Command::Verify: {
                return onVrfy(session, arg);
            }
            case Command::ETrn:
            case Command::ATrn: {
                return onEtrn(session, arg);
            }
            case Command::Reset: {
                return reset(session, arg);
            }
            case Command::Noop: {
                return noop(session);
            }
            case Command::StartTls: {
                return onStartTls(session, arg);
            }
            case Command::Expn: {
                return onExpn(session, arg);
                break;
            }
            case Command::Unrecognized:
            default: {
                return Response::create(500, "Unrecognised command");
            }
        }
    }

    Response ServerContext::hello(Session& session, const String& domain, bool isEhlo)
    {
        static String Welcome{suil::catstr(hostname(), " at your service")};
        if (domain.empty()) {
            session.nextState(Session::State::Quit);
            return Response::abort(501, "Empty HELO/EHLO argument not allowed, closing connection");
        }

        if (!validateDomain(domain)) {
            session.nextState(Session::State::Quit);
            return Response::abort(501, "Unsupported HELO/EHLO argument");
        }

        auto nextState = (session.getState() == Session::State::ExpectHello) ?
                         Session::State::ExpectAuth : session.getState();
        session.reset(nextState);
        if (!isEhlo) {
            return Response::create(250, Welcome.peek());
        }

        Response resp = Response::create(250, Welcome.peek());
        for (const auto& ext: extensions()) {
            // Add supported extensions
            resp.push(ext.peek());
        }

        return std::move(resp);
    }

    Response ServerContext::quit(Session& session, const String& arg)
    {
        if (arg) {
            // command does not accept arguments
            return Response::create(501, "Syntax error, QUIT does not accept arguments");
        }
        session.reset();
        _GUARDED(onQuit, session);
        session.nextState(Session::State::Quit);
        return Response::create(221, "Closing connection! goodbye");
    }

    Response ServerContext::startEmail(Session& session, const String& from)
    {
        Response  resp;
        if (!expectState(resp, session, Session::State::ExpectMail)) {
            return std::move(resp);
        }

        session.expectedSize = 0;
        String tmp;
        session.expectedSize = parseMailFrom(from, tmp);
        if (session.expectedSize < 0) {
            // Parsing email failed
            return tmp.empty()?
                        Response::create(553, "Syntax error, invalid email address") :
                        Response::create(555, "Syntax error, unsupported parameters");
        }

        session.from = tmp.dup();
        // Mail from received
        if (!guarded(resp, Ego, &ServerContext::onMailFrom, session)) {
            // Out of our handler, only the post master would know
            session.from = {};
            return Response::create(555, "Kaboom!!!! MAIL FROM contact postmaster");
        }
        if (resp.code() == 250) {
            // MAIL FROM accepted, wait for recipients
            session.nextState(Session::State::ExpectRecipient);
        }

        return std::move(resp);
    }

    Response ServerContext::addRecipient(Session& session, const String& line)
    {
        Response  resp;
        if (!expectState(resp, session, Session::State::ExpectRecipient)) {
            return std::move(resp);
        }

        String addr;
        if (!extractEmail(line, addr).empty()) {
            return Response::create(553, "Syntax error, invalid email address");
        }

        session.recipients.push_back(addr.dup());
        if (!guarded(resp, Ego, &ServerContext::onMailRcpt, session)) {
            session.recipients.pop_back();
            // Out of our hands, leave it up to the postmaster
            return Response::create(555, "Kaboom!!!! RCP TO contact postmaster");
        }

        // We need to remain in this state even on errors
        return std::move(resp);
    }

    Response ServerContext::startData(Session& session, const String& arg)
    {
        if (arg) {
            // DATA command does not accept an argument
            return Response::create(501, "Syntax error, DATA does not accept parameters");
        }

        Response  resp;
        if (!expectState(resp, session, Session::State::ExpectRecipient)) {
            return std::move(resp);
        }

        if (!guarded(resp, Ego, &ServerContext::onDataStart, session)) {
            // Unhandled errors our out of our hands
            return Response::create(555, "Kaboom!!!! DATA contact postmaster");
        }
        if (resp.code() == 354) {
            // DATA command has been accepted, advance to next state
            session.nextState(Session::State::ExpectData);
        }

        return std::move(resp);
    }

    Response ServerContext::onDataStart(Session& session)
    {
        Response ret;
        if (!expectState(ret, session, Session::State::ExpectRecipient)) {
            return std::move(ret);
        }

        // Create a file to handle the incoming message
        _GUARDED(session.data<DefaultSessionData>().open);

        return Response::create(354, "OK send data terminated by CRLF.CRLF");
    }

    void ServerContext::onDataPart(Session& session)
    {
        // Append the incoming message to a file
        _GUARDED(
                session.data<DefaultSessionData>()().write,
                session.rxd.data(),
                session.rxd.size());
    }

    Response ServerContext::onDataTerm(Session& session)
    {
        _GUARDED(session.data<DefaultSessionData>().close);
        return Response::create(250, "OK message received");
    }

    Response ServerContext::onMailFrom(Session& session)
    {
        return Response::create(250, "OK give me recipients");
    }

    Response ServerContext::onMailRcpt(Session& session)
    {
        return Response::create(250, "OK give more recipients");
    }

    Response ServerContext::onAuthUsernamePasswd(const String& username, const String& passwd)
    {
        return Response::create(235, "OK Authorized, ready when you are");
    }

    Response ServerContext::onVrfy(Session& session, const String& what)
    {
        return Response::create(502, "Unimplemented command");
    }

    Response ServerContext::onExpn(Session& session, const String& arg)
    {
        return Response::create(502, "Unimplemented command");
    }

    Response ServerContext::onEtrn(Session& session, const String& where)
    {
        return Response::create(502, "Unimplemented command");
    }

    Response ServerContext::onStartTls(Session& session, const String& arg)
    {
        return Response::create(502, "Unimplemented command");
    }

    void ServerContext::onReset(Session& session)
    {
    }

    void ServerContext::onQuit(Session& session)
    {
    }

    Response ServerContext::noop(Session& session)
    {
        return Response::create(250, "OK");
    }

    Response ServerContext::onHelp(Session& session, const String& what)
    {
        _GUARDED(onHelp, session, what);
        return Response::create(214, "See https://tools.ietf.org/html/rfc5321 for more info");
    }

    Response ServerContext::reset(Session& session, const String& arg)
    {
        // Invoke handler first followed
        _GUARDED(onReset, session);
        _GUARDED(session.reset, session.getState());

        return Response::create(250, "OK");
    }

    Response ServerContext::startAuth(Session& session, const String& arg)
    {
        Response resp;
        if (!expectState(resp, session, Session::State::ExpectAuth)) {
            return std::move(resp);
        }

        if (arg.empty()) {
            // Auth mechanism needed with AUTH command
            return  Response::create(501, "Empty AUTH argument not allowed");
        }

        // expect AUTH PROTOCOL optional
        auto space = arg.find(' ');
        String proto = arg.peek();
        String auth{};
        if (space > 0) {
            proto = arg.substr(0, space++);
            auth  = arg.substr(space);
        }

        // should verify
        session.auth.mechanism = parseAuthMechanism(proto);
        return protocolAuth(session, auth);
    }

    Response ServerContext::protocolAuth(Session& session, const String& arg) {
        switch (session.auth.mechanism) {
            case AuthMechanism::Plain: {
                if (arg) {
                    // auth data provided, i.e AUTH <MECHANISM> <ARG0>
                    return authenticateUser(session.auth.mechanism, {}, arg);
                } else {
                    // request auth data
                    session.nextState(Session::State::AuthExpectPassword);
                    return Response::create(334, "");
                }
            }
            case AuthMechanism::Login: {
                if (arg) {
                    // username provided, i.e AUTH <MECHANISM> <ARG0>
                    session.auth.arg0 = arg.dup();
                    session.nextState(Session::State::AuthExpectPassword);
                    return  Response::create(334, PasswordB64.peek());
                } else {
                    session.nextState(Session::State::AuthExpectUsername);
                    return Response::create(334, UsernameB64.peek());
                }
            }
            case AuthMechanism::PlainToken:
            case AuthMechanism::OAuthBearer:
            case AuthMechanism::DigestMD5:
            case AuthMechanism::MD5:
            case AuthMechanism::CramMD5:
            case AuthMechanism::Unknown:
            default:
                return Response::create(504, "Unrecognized auth mechanism");
        }
    }

    Response ServerContext::onLine(Session& session, const String& line)
    {
        if (session.getState() == Session::State::AuthExpectUsername) {
            // received username
            session.auth.arg0 = line.dup();
            session.nextState(Session::State::AuthExpectPassword);
            return Response::create(334, PasswordB64.peek());
        }
        else if (session.getState() == Session::State::AuthExpectPassword) {
            // received password/token
            auto resp = authenticateUser(session.auth.mechanism, session.auth.arg0, line);
            if (resp.code() != 235) {
                // Authentication failed
                session.nextState(Session::State::ExpectAuth);
            }
            else {
                // Authentication success
                session.nextState(Session::State::ExpectMail);
            }

            return std::move(resp);
        }
        else {
            // shouldn't happen
            return Response::create(502, "Issue AUTH first");
        }
    }

    std::optional<Response> ServerContext::onData(Session& session, const Data& data)
    {
        Response resp;
        if (!expectState(resp, session, Session::State::ExpectData)) {
            return std::move(resp);
        }

        auto done = isTermination(session, data);
        if (done) {
            // take a snapshot of the data
            session.rxd = Data{data.data(), data.size(), false};
            _GUARDED(onDataPart, session);
            session.nextState(Session::State::ExpectMail);
            if (!guarded(resp, Ego, &ServerContext::onDataTerm, session)) {
                // Beyond our scope, leave it to postmaster
                return Response::create(553, "Kaboom!!!! RCP TO contact postmaster");
            }
            return std::move(resp);
        }
        else {
            // pass on the data
            session.rxd = data.peek();
            _GUARDED(onDataPart, session);
            return {};
        }
    }

    Response ServerContext::authenticateUser(
            AuthMechanism mechanism,
            const String& arg0,
            const String& arg1)
    {
        switch (mechanism) {
            case AuthMechanism::Login: {
                return authLogin(arg0, arg1);
            }
            case AuthMechanism::Plain: {
                return authPlain(arg1);
            }
            case AuthMechanism::CramMD5: {
                return authCramMD5(arg1);
            }
            case AuthMechanism::MD5: {
                return authMD5(arg1);
            }
            case AuthMechanism::DigestMD5: {
                return authDigestMD5(arg1);
            }
            case AuthMechanism::OAuthBearer: {
                return onAuthOAuthBearer(arg1);
            }
            case AuthMechanism::PlainToken: {
                return onAuthPlainToken(arg1);
            }
            default: {
                return Response::create(504, "Unrecognized authentication type");
            }
        }
    }

    Response ServerContext::authLogin(const String& username, const String& passwd)
    {
        try {
            auto plainUsername = Base64::decode(username);
            auto plainPasswd = Base64::decode(passwd);
            return onAuthUsernamePasswd(plainUsername, plainPasswd);
        }
        catch (...) {
            auto ex =  Exception::fromCurrent();
            idebug("LOGIN: error decoding username password: %s", ex.what());
            return Response::create(501, "Cannot decode response");
        }
    }

    Response ServerContext::authPlain(const String& plain)
    {
        try {
            auto userPasswd = Base64::decode(plain);
            String id{}, user{}, passwd{};
            if (userPasswd.empty()) {
                idebug("AUTH PLAIN empty username:password string");
                return Response::create(501, "Username and Password not accepted");
            }
            ssize_t pos{0};
            if (userPasswd[pos++] != '\0') {
                auto tmp = userPasswd.find('\0');
                if (tmp > 0) {
                    id = userPasswd.substr(0, tmp);
                    pos = tmp+1;
                }
            }
            auto tmp = userPasswd.substr(pos).find('\0');
            if (tmp > 0) {
                user = userPasswd.substr(pos, (tmp-pos));
                pos = tmp+1;
            }
            passwd = userPasswd.substr(pos);
            if (user.empty() or passwd.empty()) {
                idebug("AUTH PLAIN username/password empty");
                return Response::create(501, "Username and Password not accepted");
            }

            return onAuthUsernamePasswd(user, passwd);
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            idebug("AUTH PLAIN: error decoding string: %s", ex.what());
            return Response::create(501, "Cannot decode response");
        }
    }

    Response ServerContext::onAuthOAuthBearer(const String& token)
    {
        return Response::create(504, "Authentication not supported");
    }

    Response ServerContext::onAuthPlainToken(const String& token)
    {
        return Response::create(504, "Authentication not supported");
    }

    Response ServerContext::authMD5(const String& md5)
    {
        return Response::create(504, "Authentication not supported");
    }

    Response ServerContext::authCramMD5(const String& cramMD5)
    {
        return Response::create(504, "Authentication not supported");
    }

    Response ServerContext::authDigestMD5(const String& digest)
    {
        return Response::create(504, "Authentication not supported");
    }

    int ServerContext::parseMailFrom(const String& line, String& addr) const
    {
        auto tmp = extractEmail(line, addr);
        if (tmp.empty()) {
            return addr.empty()? -1: 0;
        }

        if ((tmp = eat(tmp, 0)).empty()) {
            // consume all spaces
            return 0;
        }

        // Beyond this point we expect a SIZE argument
        if (tmp.size() < 4) {
            // SIZE is a least 4 bytes
            return -1;
        }

        if (strncasecmp(&tmp[0], "SIZE", 4) != 0) {
            // Expecting size argument
            return -1;
        }

        if ((tmp = eat(tmp, 4)).empty()) {
            // Expecting the argument to size
            return -1;
        }

        try {
            int size{};
            suil::cast(tmp, size);
            return size;
        }
        catch (...) {
            return -1;
        }
    }

    String ServerContext::extractEmail(const String& line, String& addr) const
    {
        String it = line.peek();
        if ((it = eat(it, 0)).empty() or (it[0] != ':')) {
            // expect optional spaces followed by ':'
            return {};
        }

        if ((it = eat(it, 1)).empty() or (it[0] != '<')) {
            // expect spaces followed by '<'
            return {};
        }

        auto tmp = capture(it, 1, isChar('>'));
        if (tmp.empty()) {
            return {};
        }

        if (!Email::Address::validate(tmp)) {
            // email address invalid
            return {};
        }

        addr = tmp;
        // skip the '<',  captured, '>' characters
        return it.substr(2+tmp.size());
    }

    bool ServerContext::validateDomain(const String& domain)
    {
        return true;
    }

    bool ServerContext::expectState(Response& resp, const Session& sess, Session::State expected)
    {
        if (sess.state == expected) {
            return true;
        }

        switch (sess.state) {
            case Session::State::ExpectHello:
                resp =Response::create(503, "Issue HELO/EHLO first");
                break;
            case Session::State::ExpectMail:
                resp = Response::create(503, "Issue MAIL FROM first");
                break;
            case Session::State::ExpectRecipient:
                resp = Response::create(503, "Issue RCPT TO first");
                break;
            case Session::State::ExpectAuth:
                resp = Response::create(503, "Issue AUTH first");
                break;
            default:
                resp = Response::create(503, "Unexpected command");
                break;
        }
        return false;
    }

    const std::set<String> & ServerContext::extensions() const
    {
        return mExtesions;
    }

    bool ServerContext::isTermination(Session& sess, const Data& data) {
        auto pick = std::min(data.size(), size_t(5));

        sess.term.erase(sess.term.begin(),
                        sess.term.begin() + std::min(sess.term.size(), pick));
        for (auto i = data.size() - pick; i < data.size(); i++) {
            sess.term.push_back(data.data()[i]);
        }

        if (sess.term.size() < 5) {
            return false;
        }

        return memcmp(&sess.term[0], "\r\n.\r\n", 5) == 0;
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

using suil::String;
using suil::net::smtp::ServerContext;
using suil::net::smtp::Session;

TEST_CASE("Using SmtpContext", "[smtp][context]")
{
    SECTION("Invoking ServerContent::extractEmail") {
        ServerContext ctx;
        WHEN("Passing valid addresses") {
            auto _verify = [&](const String& line, const String& email, const String& lo) {
                String addr;
                auto tmp = ctx.extractEmail(line, addr);
                REQUIRE(addr == email);
                REQUIRE(tmp == lo);
            };

            _verify(": <lol@lol.com>", "lol@lol.com", "");
            _verify(" :<lol@lol.com>", "lol@lol.com", "");
            _verify(" : <lol@lol.com> ", "lol@lol.com", " ");
        }

        WHEN("Passing invalid address lines") {
            auto _verify = [&](const String& line) {
                String addr;
                auto tmp = ctx.extractEmail(line, addr);
                REQUIRE(tmp.empty());
                REQUIRE(addr.empty());
            };

            _verify(":: <lol@lol.com>");
            _verify(" <lol@lol.com>");
            _verify(" lol@lol.com");
            _verify(":<lol@lol.com");
            _verify("<lol@lol.com>");
            _verify(": <lol@lol.com ");
        }
    }

    WHEN("Invoking ServerContext::ParseMailFrom") {
        ServerContext ctx;
        WHEN("Passing valid MAIL FROM lines") {
            String addr;
            auto size = ctx.parseMailFrom(": <lol@lol.com>", addr);
            REQUIRE(size == 0);
            REQUIRE(addr == "lol@lol.com");
            size = ctx.parseMailFrom(": <lol@lol.com> size 20", addr);
            REQUIRE(size == 20);
            REQUIRE(addr == "lol@lol.com");
            size = ctx.parseMailFrom(": <lol@lol.com> SIZE 200", addr);
            REQUIRE(size == 200);
            REQUIRE(addr == "lol@lol.com");
        }

        WHEN("Passing in valid MAIL FROM lines") {
            String addr;
            auto size = ctx.parseMailFrom(": <lol@lol.com> DIZE 100", addr);
            REQUIRE(size == -1);
            size = ctx.parseMailFrom(": <lol@lol.com> size r20", addr);
            REQUIRE(size == -1);
            size = ctx.parseMailFrom(": <lol@lol.com> SIZE", addr);
            REQUIRE(size == -1);
            size = ctx.parseMailFrom(": <lol@lol.com> SIZE 100 AUTH", addr);
            REQUIRE(size == -1);
        }
    }

    SECTION("When invoking ServerContext::isTermination") {
        ServerContext ctx;
        Session sess;
        auto _d  = [](const char* in) { return suil::Data{in, strlen(in), false}; };
        auto _has = [&sess](const String& str) {
            return String{&sess.term[0], sess.term.size(), false} == str;
        };
        auto status = ctx.isTermination(sess, _d("Hello beautiful world\r\n"));
        REQUIRE_FALSE(status);
        REQUIRE(_has("rld\r\n"));
        status = ctx.isTermination(sess, _d("Hello beautiful world again\r\n"));
        REQUIRE_FALSE(status);
        REQUIRE(_has("ain\r\n"));
        status = ctx.isTermination(sess, _d("\r\n"));
        REQUIRE_FALSE(status);
        REQUIRE(_has("n\r\n\r\n"));
        status = ctx.isTermination(sess, _d("."));
        REQUIRE_FALSE(status);
        REQUIRE(_has("\r\n\r\n."));
        status = ctx.isTermination(sess, _d("\r\n"));
        REQUIRE(status);
        REQUIRE(_has("\r\n.\r\n"));
    }
}
#endif