//
// Created by Mpho Mbotho on 2020-12-29.
//

#include "suil/http/client/session.hpp"

#include <suil/net/ssl.hpp>
#include <suil/net/tcp.hpp>

namespace suil::http::cli {

    Session::Handle::Handle(Session& sess, net::Socket::UPtr sock)
        : _session{sess},
          req{std::move(sock)}
    {}

    Session::Handle::Handle(Handle&& o)
        : _session{o._session},
          req{std::move(o.req)}
    {}

    Session::Handle& Session::Handle::operator=(Handle&& o)
    {
        if (this == &o) {
            return Ego;
        }
        _session = o._session;
        req = std::move(o.req);

        return Ego;
    }

    Session::Session(String proto, String host, int port)
        : _proto{std::move(proto)},
          _host{std::move(host)},
          _port{port}
    {}

    void Session::header(String name, String value)
    {
        _headers.emplace(std::move(name), std::move(value));
    }

    Session::Handle Session::handle()
    {
        net::Socket::UPtr sock{nullptr};
        if (isHttps()) {
            sock = std::make_unique<net::SslSock>();
        }
        else {
            sock = std::make_unique<net::TcpSock>();
        }

        return Handle{Ego, std::move(sock)};
    }

    void Session::keepAlive(bool on)
    {
        if (on) {
            header("Connection", "Keep-Alive");
        }
        else {
            header("Connection", "Close");
        }
    }

    void Session::connect(Handle& h, const UnorderedMap<String, CaseInsensitive>& headers)
    {
        auto& req = h.req;
        if (!req._sock->isOpen()) {
            // request socket not connected to server
            if (!req._sock->connect(_addr, _timeout)) {
                throw HttpException("Socket connection to '", _host, ':', _port, "' failed: ", errno_s);
            }
        }

        auto resp = perform(Method::Connect, h, "/", [&](Request& req) -> bool {
            for (auto& hdr: headers) {
                req.header(hdr.first.peek(), hdr.second.peek());
            }
            return true;
        }, nullptr);

        if (resp.status() != http::Ok) {
            // connecting to server failed
            throw HttpException("Connecting to server '", _host, ':', _port, "' failed: ",
                            toString(resp.status()));
        }
        h.req.cleanup();
    }
    Response Session::head(
            Handle& h, String resource, const UnorderedMap<String, CaseInsensitive>& headers)
    {
        return perform(Method::Connect, h, std::move(resource), [&](Request& req) -> bool {
            for (auto& hdr: headers) {
                req.header(hdr.first.peek(), hdr.second.peek());
            }
            return true;
        }, nullptr);
    }

    Response Session::perform(
                Method m,
                Handle& h,
                String resource,
                Request::Builder builder,
                Response::Writer writer)
    {
        auto& req = h.req;
        Response resp;
        req.reset(m, std::move(resource), false);
        for (auto& hdr : _headers) {
            req.header(hdr.first.peek(), hdr.second.peek());
        }

        if (builder != nullptr) {
            // invoke request builder
            if (!builder(req)) {
                throw HttpException("Building request for resource failed failed");
            }
        }

        if (unlikely(!req._sock->isOpen())) {
            // Open connection, very unlikely
            if (!req._sock->connect(_addr, _timeout)) {
                throw HttpException("Socket connection to '", _host, ':', _port, "' failed: ", errno_s);
            }
        }

        req.submit(_timeout);
        resp._writer = std::move(writer);
        resp.receive(*req._sock, _timeout);
        return std::move(resp);
    }

    Response Session::perform(Handle& h, Method m, String resource)
    {
        Request::Builder builder{nullptr};
        Response::Writer writer{nullptr};
        return perform(m, h, std::move(resource), builder, writer);
    }

    Session Session::doLoad(const String& url, int port, String path)
    {
        auto pos = url.find("://");
        String proto{}, host{};
        if (pos != String::npos) {
            /* protocol is part of the URL */
            proto = url.substr(0, pos).dup();
            host  = url.substr(pos+3).dup();
        }
        else {
            /* use default protocol */
            proto = String("http").dup();
            host  = String(url).dup();
        }
        // @TODO load session for given path
        return Session{std::move(proto), std::move(host), port};
    }
}