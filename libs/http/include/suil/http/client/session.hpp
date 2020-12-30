//
// Created by Mpho Mbotho on 2020-12-29.
//

#ifndef SUIL_HTTP_CLIENT_SESSION_HPP
#define SUIL_HTTP_CLIENT_SESSION_HPP

#include <suil/http/client/request.hpp>
#include <suil/http/client/response.hpp>
#include <suil/http/client.scc.hpp>

#ifndef SUIL_HTTP_USER_AGENT
#define SUIL_HTTP_USER_AGENT SUIL_SOFTWARE_NAME "/" SUIL_VERSION_STRING
#endif

namespace suil::http::cli {

    class Session: LOGGER(HTTP_CLIENT) {
    public:
        class Handle {
        public:
            Handle(Session& sess, net::Socket::UPtr sock);

            MOVE_CTOR(Handle);
            MOVE_ASSIGN(Handle);

            DISABLE_COPY(Handle);

            inline operator bool() const {
                return req._sock->isOpen();
            }

            inline  Session& operator()() {
                return _session.get();
            }

            std::reference_wrapper<Session> _session;
            Request req;
        };

        MOVE_CTOR(Session) = default;
        MOVE_ASSIGN(Session) = default;

        void header(String name, String value);

        template <typename T>
        inline void header(String name, T&& value) {
            header(std::move(name), suil::tostr(std::forward<T>(value)));
        }

        inline void language(String lang) {
            header("Accept-Language", std::move(lang));
        }

        inline void userAgent(String agent) {
            header("User-Agent", std::move(agent));
        }

        void keepAlive(bool on = true);

        Handle handle();

        inline Handle connect(const UnorderedMap<String,CaseInsensitive>& headers = {}) {
            auto h = handle();
            connect(h, headers);
            return h;
        }

        void connect(Handle& h, const UnorderedMap<String,CaseInsensitive>& headers = {});

        Response head(Handle& h, String resource, const UnorderedMap<String,CaseInsensitive>& headers = {});

        template <typename... Options>
        static Session load(String site, int port, String path, Options&&... options) {
            auto session = Session::doLoad(site, port, std::move(path));
            session.configure(std::forward<Options>(options)...);
            return session;
        }

        Response perform(
                Method m,
                Handle& h,
                String resource,
                Request::Builder builder,
                Response::Writer writer = nullptr);

        Response perform(
                Handle& h,
                Method m,
                String resource);

    private:
        DISABLE_COPY(Session);
        Session(String proto, String host, int port);

        static Session doLoad(const String& url, int port, String path);

        template<typename... Options>
        void configure(Options&&... options) {
            // @TODO read session from path
            auto opts = iod::D(std::forward<Options>(options)...);
            _timeout = opts.get(sym(timeout), _timeout);

            if (_port == 0) {
                // choose port base on standard ports
                _port = isHttps()? 443 : 80;
            }
            _addr = ipremote(_host.data(), _port, 0, Deadline{_timeout});
            if (errno != 0) {
                throw HttpException("resolving address '", _host, ':', _port, "' failed: ", errno_s);
            }
            header("Host", _host.peek());
            userAgent(SUIL_HTTP_USER_AGENT);
            language("en-US");
        }

        inline bool isHttps() const {
            return _proto == "https";
        }

        int _port{80};
        String _host{};
        UnorderedMap<String, CaseInsensitive> _headers{};
        int64 _timeout{20_sec};
        ipaddr _addr{};
        String _proto{"http"};
    };

    template <typename... Options>
    inline Session loadSession(String site, int port, String path, Options&&... options) {
        return Session::load(std::move(site), port, path, std::forward<Options>(options)...);
    }

    inline Response get(Session::Handle& h,String resource, Request::Builder builder = nullptr ) {
        return h().perform(Method::Get, h, std::move(resource), std::move(builder));
    }

    inline Response get(Session& sess, String resource, Request::Builder builder = nullptr ) {
        auto h = sess.handle();
        return h().perform(Method::Get, h, std::move(resource), std::move(builder));
    }

    template <typename T>
    inline Response get(T& off, Session::Handle& h, String resource, Request::Builder builder = nullptr ) {
        return h().perform(Method::Get, h, std::move(resource), std::move(builder), off());
    }

    template <typename T>
    inline Response get(T& off,Session& sess, String resource, Request::Builder builder = nullptr ) {
        auto h = sess.handle();
        return sess.perform(Method::Get, h, std::move(resource), std::move(builder), off());
    }

    inline Response post(Session::Handle& h, String resource, Request::Builder builder = nullptr) {
        return h().perform(Method::Post, h, std::move(resource), std::move(builder));
    }

    inline Response post(Session& sess, String resource, Request::Builder builder = nullptr) {
        auto h = sess.handle();
        return h().perform(Method::Post, h, std::move(resource), std::move(builder));
    }

    inline Response put(Session::Handle& h, String resource, Request::Builder builder = nullptr) {
        return h().perform(Method::Put, h, std::move(resource), std::move(builder));
    }

    inline Response put(Session& sess, String resource, Request::Builder builder = nullptr) {
        auto h = sess.handle();
        return sess.perform(Method::Put, h, std::move(resource), std::move(builder));
    }

    inline Response del(Session::Handle& h,String resource, Request::Builder builder = nullptr) {
        return h().perform(Method::Delete, h, std::move(resource), std::move(builder));
    }
    inline Response del(Session& sess, String resource, Request::Builder builder = nullptr) {
        auto h = sess.handle();
        return sess.perform(Method::Delete, h, std::move(resource), std::move(builder));
    }

    inline Response options(Session::Handle& h, String resource, Request::Builder builder = nullptr) {
        return h().perform(Method::Options, h, std::move(resource), std::move(builder));
    }

    inline Response options(Session& sess,String resource, Request::Builder builder = nullptr) {
        auto h = sess.handle();
        return sess.perform(Method::Options, h, std::move(resource), std::move(builder));
    }

    inline Response head(Session::Handle& h, String resource, Request::Builder builder = nullptr) {
        return h().perform(Method::Head, h, std::move(resource), std::move(builder));
    }

    inline Response head(Session& sess, String resource, Request::Builder builder = nullptr) {
        auto h = sess.handle();
        return sess.perform(Method::Head, h, std::move(resource), std::move(builder));
    }
}
#endif //SUIL_HTTP_CLIENT_SESSION_HPP
