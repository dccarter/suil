//
// Created by Mpho Mbotho on 2020-12-29.
//

#include "suil/http/client/request.hpp"
#include <suil/base/datetime.hpp>

namespace suil::http::cli {

    Request::Request(net::Socket::UPtr sock)
        : _sock{std::move(sock)}
    {}

    Request::~Request()
    {
        if (_sock != nullptr) {
            // this will implicitly end the connection
            _sock = nullptr;
        }

        cleanup();
    }

    void Request::header(String name, String value)
    {
        _headers.emplace(std::move(name), std::move(value));
    }

    void Request::arg(String name, String value)
    {
        _arguments.emplace(std::move(name), Base64::urlEncode(value));
    }

    void Request::setKeepAlive(bool on)
    {
        if (on) {
            header("Connection", "Keep-Alive");
        }
        else {
            header("Connection", "Close");
        }
    }

    Request& Request::operator<<(Form&& form)
    {
        if (!_body.empty() or _bodyFd.valid()) {
            throw HttpException("request body already set as ", (_bodyFd.valid()? "file" : "buffer"));
        }

        _form = std::move(form);
        if (_form._encoding == Form::UrlEncoded) {
            header("Content-Type", "application/x-www-form-urlencoded");
        }
        else {
            header("Content-Type",
                   suil::catstr("multipart/form-data; boundary=", _form._boundary));
        }

        return Ego;
    }

    Request& Request::operator<<(File&& body)
    {
        if (!_body.empty() or _form) {
            throw HttpException("request body already set as ", (_body.empty()? "buffer": "form"));
        }
        _bodyFd = std::move(body);
        return Ego;
    }

    Buffer& Request::operator()(String contentType)
    {
        if (_bodyFd.valid() or _form) {
            throw HttpException("request body already set as ", (_bodyFd.valid()? "form" : "file"));
        }

        header("Content-Type", std::move(contentType));
        return _body;
    }

    uint8* Request::buffer(size_t size, String contentType)
    {
        Ego(std::move(contentType));
        _body.reserve(size);
        auto pos = _body.size();
        _body.seek(size);
        return &_body[pos];
    }

    void Request::reset(Method m, String res, bool clear)
    {
        if (clear or m != _method or res != _resource) {
            cleanup();
            _resource = std::move(res);
            _method = m;
        }
    }

    void Request::cleanup()
    {
        _arguments.clear();
        _form.clear();
        _body.clear();
        _headers.clear();
        _bodyFd.close();
        _resource.clear();
    }

    void Request::encodeArgs(Buffer& dst) const
    {
        if (_arguments.empty()) {
            return;
        }

        dst << '?';
        for (auto it = _arguments.begin(); it != _arguments.end(); it++) {
            if (it != _arguments.begin()) {
                dst << '&';
            }
            dst << it->first << '=' << it->second;
        }
    }

#undef  CRLF
#define CRLF "\r\n"

    void Request::encodeHeaders(Buffer& dst) const
    {
        for (auto& hdr: _headers) {
            dst << hdr.first << ": " << hdr.second << CRLF;
        }
    }

    size_t Request::buildBody()
    {
        if (_bodyFd.valid()) {
            // just return size of file
            return _bodyFd.tell();
        }

        if (_form) {
            // encode form into _body
            return _form.encode(_body);
        }

        return _body.size();
    }

    void Request::submit(int64 timeout)
    {
        Buffer head{2048};
        auto contentLength = buildBody();
        if (contentLength > 0 and
            !suil::matchany(_method, Method::Put, Method::Post))
        {
            swarn("sending non Put/Post request with payload might not be supported by destination server");
        }

        header("Content-Length", contentLength);
        head << http::toString(_method) << " " << _resource;
        encodeArgs(head);
        head << " HTTP/1.1" << CRLF;
        header("Date", String{Datetime{}(Datetime::HTTP_FMT)}.dup());
        encodeHeaders(head);
        head << CRLF;

        // send headers
        auto writen = _sock->send(head.data(), head.size(), timeout);
        if (writen != head.size()) {
            // sending headers failed
            throw HttpException("Sending request headers failed: ", errno_s);
        }

        if (contentLength == 0) {
            // nothing to send
            _sock->flush(timeout);
            return;
        }

        if (Ego._bodyFd.valid()) {
            // send body as file
            auto fd = Ego._bodyFd.raw();
            writen = _sock->sendfile(fd, 0, contentLength, timeout);
            if (writen != contentLength) {
                // sending file failed
                throw HttpException("sending request body-fd file failed: ", errno_s);
            }
            return;
        }

        if (!_body.empty()) {
            // send body
            writen = _sock->send(_body.data(), _body.size(), timeout);
            if (writen != _body.size()) {
                // sending body buffer failed
                throw HttpException("sending request body failed: ", errno_s);
            }
        }

        if (!_form._uploads.empty()) {
            // submit uploaded files
            for (auto& up: _form._uploads) {
                auto fd = up.open();
                writen = _sock->send(up.getHead().data(), up.getHead().size(), timeout);
                if (writen != up.getHead().size()) {
                    // sending upload head failed
                    throw HttpException("sending request upload failed: ", errno_s);
                }
                size_t totalWritten{0};
                do {
                    writen = _sock->sendfile(fd, totalWritten, up.size()-totalWritten, timeout);
                    if (errno) {
                        // sending upload file failed
                        throw HttpException("sending request upload failed: ", errno_s);
                    }
                    totalWritten += writen;
                } while (totalWritten != up.size());

                if (_sock->send(CRLF, sizeofcstr(CRLF), timeout) != sizeofcstr(CRLF)) {
                    throw HttpException("send CRLF after upload failed: ", errno_s);
                }

                /* close boundary*/
                up.close();
            }

            head.reset(1023, true);
            head << "--" << _form._boundary << "--"
                 << CRLF;

            writen = _sock->send(head.data(), head.size(), timeout);
            if (writen != head.size()) {
                // sending terminal boundary failed
                throw HttpException("Sending terminal boundary failed: ", errno_s);
            }
        }

        _sock->flush(timeout);
    }
}