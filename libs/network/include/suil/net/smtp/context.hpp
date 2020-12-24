//
// Created by Mpho Mbotho on 2020-11-23.
//

#ifndef SUILNETWORK_CONTEXT_HPP
#define SUILNETWORK_CONTEXT_HPP

#include "suil/net/smtp/command.hpp"
#include "suil/net/smtp/response.hpp"
#include "suil/net/smtp/session.hpp"
#include "suil/net/config.scc.hpp"

#include <suil/base/string.hpp>

#include <set>
#include <utility>

namespace suil::net::smtp {

    using ApiReturn = std::pair<Session::State, Response>;
    define_log_tag(SMTP_CONTEXT);

    class ServerContext : public LOGGER(SMTP_CONTEXT) {
    public:
        sptr(ServerContext);
        virtual const String& hostname();
        virtual bool isReady();
        Response onCommand(
                Session& session,
                Command cmd,
                const String& arg);
        Response onConnect(
                Session& session,
                const String& id);
        Response onLine(
                Session& session,
                const String& line);
        std::optional<Response> onData(
                Session& session,
                const Data& data);

        const SmtpServerConfig& config();
        void addExtension(String extension);
    protected suil_ut:
        virtual Response onMailFrom(Session& session);
        virtual Response onMailRcpt(Session& session);
        virtual Response onDataStart(Session& session);
        virtual void onDataPart(Session& session);
        virtual Response onDataTerm(Session& session);
        virtual Response onHelp(Session& session, const String& what);
        virtual Response onVrfy(Session& session, const String& what);
        virtual Response onEtrn(Session& session, const String& where);
        virtual Response onExpn(Session& session, const String& arg);
        virtual void onReset(Session& session);
        virtual void onQuit(Session& session);
        virtual Response onStartTls(Session& session, const String& arg);
        virtual Response onAuthOAuthBearer(const String& token);
        virtual Response onAuthPlainToken(const String& token);
        virtual Response onAuthUsernamePasswd(const String& username, const String& passwd);

    private suil_ut:
        virtual bool validateDomain(const String& domain);
        const std::set<String>& extensions() const;

    private suil_ut:
        Response quit(Session& session, const String& arg);
        Response noop(Session& session);
        Response reset(Session& session, const String& arg);
        Response protocolAuth(Session& session, const String& arg);
        Response startAuth(Session& session, const String& arg);
        Response hello(Session& session, const String& domain, bool isEhlo);
        Response startEmail(Session& session, const String& from);
        Response addRecipient(Session& session, const String& from);
        Response startData(Session& session, const String& arg);
        Response authenticateUser(AuthMechanism mechanism, const String& arg0, const String& arg1);
        Response authLogin(const String& username, const String& passwd);
        Response authPlain(const String& plain);
        Response authCramMD5(const String& cramMD5);
        Response authMD5(const String& md5);
        Response authDigestMD5(const String& digest);

    private suil_ut:
        int parseMailFrom(const String& line, String& addr) const;
        String extractEmail(const String& line, String& addr) const;
        bool expectState(Response& ret, const Session& sess, Session::State expected);
        bool isTermination(Session& session, const Data& data);
    private:
        SmtpServerConfig mConfig;
        std::set<String> mExtesions;
        static const String UsernameB64;
        static const String PasswordB64;
    };
}

#endif //SUILNETWORK_CONTEXT_HPP
