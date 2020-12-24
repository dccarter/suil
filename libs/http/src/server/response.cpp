//
// Created by Mpho Mbotho on 2020-12-17.
//

#include "suil/http/server/response.hpp"

namespace suil::http::server {

    Response::Response(Status status)
        : _status{status}
    {}

    Response::Response(Buffer buffer)
        : _body{std::move(buffer)}
    {}

    Response::Response(Response&& o) noexcept
        : _chunks{std::move(o._chunks)},
          _chunksSize{o._chunksSize},
          _headers{std::move(o._headers)},
          _cookies{std::move(o._cookies)},
          _status{o._status},
          _body{std::move(o._body)},
          _completed{o._completed},
          _upgrade{std::move(o._upgrade)}
    {
        o._chunksSize = 0;
        o._completed = true;
        o._status = http::InternalError;
    }

    Response & Response::operator=(Response&& o) noexcept
    {
        Ego._chunks = std::move(o._chunks);
        Ego._chunksSize = o._chunksSize;
        Ego._headers = std::move(o._headers);
        Ego._cookies = std::move(o._cookies);
        Ego._status = o._status;
        Ego._body = std::move(o._body);
        Ego._completed = o._completed;
        Ego._upgrade = std::move(o._upgrade);
        o._chunksSize = 0;
        o._completed = true;
        o._status = http::InternalError;
        return Ego;
    }

    void Response::setContentType(String type)
    {
        header("Content-Type", std::move(type));
    }

    void Response::setCookie(Cookie ck)
    {
        if (ck) {
            Ego._cookies.emplace(ck.name().peek(), std::move(ck));
        }
    }

    void Response::header(String field, String name)
    {
        Ego._headers.emplace(std::move(field), std::move(name));
    }

    const String& Response::header(const String& name) const
    {
        auto it = Ego._headers.find(name);
        if (it == Ego._headers.end()) {
            static String Inval{};
            return Inval;
        }
        return it->second;
    }

    const String& Response::contentType() const
    {
        return Ego.header("Content-Type");
    }

    void Response::end(Status status)
    {
        Ego._status = status;
        Ego._completed = true;
    }

    void Response::end(Status status, Buffer buffer)
    {
        if (Ego._body) {
            // chunk body
            auto size = Ego._body.size();
            Ego.chunk({Ego._body.release(), size, 0, [](void *ptr) { ::free(ptr); }});
        }

        if (!Ego._chunks.empty()) {
            auto size = buffer.size();
            Ego.chunk({buffer.release(), size, 0, [](void *ptr) { ::free(ptr); }});
        }
        else {
            Ego._body =  std::move(buffer);
        }
        Ego.end(status);
    }

    void Response::end(ProtocolUpgrade upgrade)
    {
        Ego._upgrade = std::move(upgrade);
        Ego._status = http::SwitchingProtocols;
        Ego._completed = true;
    }

    void Response::redirect(Status status, String location)
    {
        header("Location", std::move(location));
        end(status);
    }

    void Response::flushCookies()
    {
        Buffer b;
        for (auto& [_, ck]: Ego._cookies) {
            if (!ck.encode(b)) {
                continue;
            }
            header("Set-Cookie", String{b});
        }
    }

    void Response::chunk(net::Chunk chunk)
    {
        Ego._chunksSize += chunk.size();
        Ego._chunks.emplace_back(std::move(chunk));
    }

    const size_t Response::size() const
    {
        return Ego._body.size() + Ego._chunksSize;
    }

    void Response::clear()
    {
        Ego.clearContent();
        Ego._completed = false;
        Ego._cookies.clear();
        Ego._headers.clear();
    }

    void Response::clearContent()
    {
        Ego._chunks.clear();
        Ego._body.clear();
        Ego._headers.erase("Content-Type");
    }

    void Response::merge(Response&& resp)
    {
        Ego._status = resp._status;
        Ego._body = std::move(resp._body);
        Ego._chunks = std::move(resp._chunks);
        Ego._cookies = std::move(resp._cookies);
        Ego._chunksSize = resp._chunksSize;
        for (auto& [k,v] : resp._headers) {
            Ego._headers.emplace(std::move(k), std::move(v));
        }
    }

}