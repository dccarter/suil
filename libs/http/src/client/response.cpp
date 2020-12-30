//
// Created by Mpho Mbotho on 2020-12-29.
//

#include "suil/http/client/response.hpp"

namespace suil::http::cli {

    Response::Response()
        : HttpParser(HTTP_RESPONSE)
    {}

    Response::Response(Response&& o)
        : HttpParser(std::move(o)),
          _bodyRead{std::exchange(o._bodyRead, false)},
          _writer{std::exchange(o._writer, nullptr)}
    {}

    Response& Response::operator=(Response&& o)
    {
        if (&o == this) {
            return Ego;
        }

        HttpParser::operator=(std::move(o));
        _bodyRead = std::exchange(o._bodyRead, false);
        _writer = std::exchange(o._writer, nullptr);

        return Ego;
    }

    const String & Response::header(const String& name) const
    {
        auto it = _headers.find(name);
        if (it == _headers.end()) {
            static String Invalid{};
            return Invalid;
        }

        return it->second;
    }

    String Response::str() const
    {
        if (_writer != nullptr) {
            // body offloaded
            return {};
        }
        return String{_stage};
    }

    Data Response::data() const
    {
        if (_writer != nullptr) {
            // body offloaded
            return {};
        }
        return Data{_stage.data(), _stage.size(), false};
    }

    void Response::receive(net::Socket& sock, int64 timeout)
    {
        char buffer[8192];
        // receive headers
        do {
            auto nread = sizeof(buffer);
            if (!sock.read(buffer, nread, timeout)) {
                throw HttpException("Receiving response failed: ", errno_s);
            }

            if (!feed(buffer, nread)) {
                throw HttpException("parsing response failed: ",
                            llhttp_get_error_reason(this));
            }
        } while (_bodyComplete != 1);
    }

    int Response::onHeadersComplete()
    {
        if (_writer == nullptr) {
            // no custom writer configured, save data in _staging area
            if (content_length) {
                _stage.reserve(content_length);
            }
            return 0;
        }

        if (_writer(nullptr, content_length)) {
            // call writer function with nullptr and content length to initialize
            return 0;
        }

        return -1;
    }

    int Response::onBodyPart(const String& part)
    {
        if (_writer == nullptr) {
            return HttpParser::onBodyPart(part);
        }

        if (_writer(part.data(), part.size())) {
            return 0;
        }

        return -1;
    }

    int Response::onMessageComplete()
    {
        if (_writer) {
            // notify writer that message is complete
            _writer(nullptr, 0);
        }
        _bodyRead = true;
        return HttpParser::onMessageComplete();
    }
}