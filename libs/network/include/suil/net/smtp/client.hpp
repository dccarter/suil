//
// Created by Mpho Mbotho on 2020-11-22.
//

#ifndef SUILNETWORK_SMTP_CLIENT_HPP
#define SUILNETWORK_SMTP_CLIENT_HPP

#include "suil/net/smtp/auth.hpp"
#include "suil/net/smtp/response.hpp"
#include "suil/net/socket.hpp"
#include "suil/net/symbols.scc.hpp"
#include "suil/net/config.scc.hpp"
#include "suil/net/email.hpp"

#include <suil/base/channel.hpp>
#include <suil/base/logging.hpp>
#include <suil/base/exception.hpp>
#include <suil/base/units.hpp>

#include <list>
#include <set>
#include <libmill/libmill.hpp>

namespace suil::net::smtp {

    define_log_tag(SMTP_CLIENT);

    DECLARE_EXCEPTION(SmtpClientError);

    class Client : LOGGER(SMTP_CLIENT) {
        using EmailAddress = Email::Address;
    public:
        template <typename ...Options>
        Client(String server, int port, Options... opts)
            : server{std::move(server)},
              port{port}
        {
            applyConfig(config, std::forward<Options>(opts)...);
        }

        Client() = default;

        void send(const Email& msg, const EmailAddress& from);

        template<typename... Params>
        bool login(Params... params) {
            if (sock != nullptr and adaptor().isOpen()) {
                // already logged in
                iwarn("already connected to mail server {%s:%d}", server(), port);
                return true;
            }

            auto opts = iod::D(params...);
            String domain = opts.get(var(domain), "localhost");
            bool forceSsl = opts.get(var(forceSsl), true);
            AuthMechanism mechanism = opts.get(var(auth), AuthMechanism::Login);

            switch (mechanism) {
                case AuthMechanism::OAuthBearer:
                case AuthMechanism::PlainToken: {
                    if (!opts.has(var(token))) {
                        ierror("selected login authentication mechanism requires a token");
                        return false;
                    }
                    return doLoginToken(
                            mechanism,
                            opts.get(var(token), ""),
                            domain, forceSsl);
                }
                default: {
                    if (not(opts.has(var(username)) or opts.has(var(passwd)))) {
                        ierror("selected login authentication mechanism requires a username and password");
                        return false;
                    }
                    return doLogin(
                            mechanism,
                            opts.get(var(username), ""),
                            opts.get(var(passwd), ""),
                            domain, forceSsl);
                }
            }
        }

        template <typename ...Options>
        void setup(String server, int port, Options... opts)
        {
            setupInit(std::move(server), port);
            suil::applyConfig(config, std::forward<Options>(opts)...);
        }

    private:
        void setupInit(String server, int port);
        void endSession();
        bool reset();
        bool createAdaptor(bool forceSsl);

        bool doLogin(
                AuthMechanism mechanism,
                const String& username,
                const String& passwd,
                const String& domain,
                bool forceSsl);
        bool doLoginToken(
                AuthMechanism mechanism,
                const String& token,
                const String& domain,
                bool forceSsl)
        { return false; }
        bool initConnection(const String& domain);
        void sendAddresses(const std::vector<EmailAddress>& addrs);
        void sendHead(const Email& msg);
        Response getResponse(const Deadline& dd);
        bool sendFlush(const Deadline& dd);
        bool sendPart(const String& part, const Deadline& dd);

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

        const char* getError(const Response& resp) const;
        const char* getError(int code) const;
        template <typename ...Parts>
        bool sendLine(const Deadline& dd, const String& part, const Parts&... parts) {
            if (!sendPart(part, dd)) {
                return false;
            }
            if constexpr (sizeof...(parts) > 0) {
                // send the rest of the parts
                sendParts(dd, parts...);
            }
            return sendFlush(dd);
        }

        bool populateExtensions(const std::vector<String>& resp);
        bool authPlain(const String& username, const String& password);
        bool authLogin(const String& username, const String& password);
        bool authMD5(const String& username, const String& password) { return false; }
        bool authDigestMD5(const String& username, const String& password) { return false; }
        bool authCramMD5(const String& username, const String& password) { return false; }

    private:
        Socket& adaptor();
        Socket::UPtr sock{nullptr};
        String server{};
        int port{0};
        SmtpClientConfig config;
        std::set<AuthMechanism> AUTH{};
        std::vector<String> EXT{};
        std::size_t SIZE{0};
    };

    class SmtpOutbox final : LOGGER(SMTP_CLIENT) {
        class Composed {
        public:
            sptr(Composed);

            Composed(Email::Ptr email, int64_t timeout);
            DISABLE_COPY(Composed);
            MOVE_CTOR(Composed);
            MOVE_ASSIGN(Composed);

            mill::Event& sync();
            Email& email();
            int64_t timeout();

        private:
            friend SmtpOutbox;
            Email::Ptr  mEmail{nullptr};
            mill::Event evSync{};
            int64_t     mTimeout{-1};
            std::atomic_bool isCancelled{false};
            String      failureMsg{};
        };
    public:
        sptr(SmtpOutbox);

        SmtpOutbox(String server, int port, Email::Address sender, std::int64_t timeout = -1);
        SmtpOutbox(Email::Address sender);

        DISABLE_COPY(SmtpOutbox);
        DISABLE_MOVE(SmtpOutbox);

        template <typename ...Params>
        bool login(Params&&... params) {
            return Ego.client.login(std::forward<Params>(params)...);
        }

        template <typename ...Params>
        void setup(String server, int port, Params... params) {
            bool expected{false};
            if (_setup.compare_exchange_strong(expected, true)) {
                mill::Lock lk{_mutex};
                Ego.client.setup(std::move(server), port);
                auto opts = iod::D(params...);
                sendTimeout = opts.get(var(timeout), sendTimeout);
            }
        }

        static Email::Ptr draft(const String& to, const String& subject) ;

        String send(Email::Ptr email, int64_t timeout = -1);

    private:
        static coroutine void sendOutbox(SmtpOutbox& Self);
        bool isQueueEmpty();
        Composed::Ptr popNextQueued();
        using SendQueue = std::list<Composed::Ptr>;
        SendQueue sendQ{};
        Client client{};
        Email::Address sender;
        std::int64_t sendTimeout{-1};
        bool quiting{false};
        std::atomic_bool _sending{false};
        std::atomic_bool _setup{false};
        mill::Mutex _mutex;
    };
}

#endif //SUILNETWORK_SMTP_CLIENT_HPP
