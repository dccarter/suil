//
// Created by Mpho Mbotho on 2020-12-14.
//

#include "suil/http/common.hpp"

namespace suil::http {

    String toString(Status status)
    {
        const char		*r;

        switch (status) {
            case Status::Continue:
                r = "HTTP/1.1 100 Continue";
                break;
            case Status::SwitchingProtocols:
                r = "HTTP/1.1 101 Switching Protocols";
                break;
            case Status::Ok:
                r = "HTTP/1.1 200 OK";
                break;
            case Status::Created:
                r = "HTTP/1.1 201 Created";
                break;
            case Status::Accepted:
                r = "HTTP/1.1 202 Accepted";
                break;
            case Status::NonAuthoritative:
                r = "HTTP/1.1 203 Non-Authoritative Information";
                break;
            case Status::NoContent:
                r = "HTTP/1.1 204 No Content";
                break;
            case Status::ResetContent:
                r = "HTTP/1.1 205 Reset Content";
                break;
            case Status::PartialContent:
                r = "HTTP/1.1 206 Partial Content";
                break;
            case Status::MultipleChoices:
                r = "HTTP/1.1 300 Multiple Choices";
                break;
            case Status::MovedPermanently:
                r = "HTTP/1.1 301 Moved Permanently";
                break;
            case Status::Found:
                r = "HTTP/1.1 302 Found";
                break;
            case Status::SeeOther:
                r = "HTTP/1.1 303 See Other";
                break;
            case Status::NotModified:
                r = "HTTP/1.1 304 Not Modified";
                break;
            case Status::UseProxy:
                r = "HTTP/1.1 305 Use Proxy";
                break;
            case Status::TemporaryRedirect:
                r = "HTTP/1.1 307 Temporary Redirect";
                break;
            case Status::BadRequest:
                r = "HTTP/1.1 400 Bad Request";
                break;
            case Status::Unauthorized:
                r = "HTTP/1.1 401 Unauthorized";
                break;
            case Status::PaymentRequired:
                r = "HTTP/1.1 402 Payment Required";
                break;
            case Status::Forbidden:
                r = "HTTP/1.1 403 Forbidden";
                break;
            case Status::NotFound:
                r = "HTTP/1.1 404 Not Found";
                break;
            case Status::MethodNotAllowed:
                r = "HTTP/1.1 405 Method Not Allowed";
                break;
            case Status::NotAcceptable:
                r = "HTTP/1.1 406 Not Acceptable";
                break;
            case Status::ProxyAuthRequired:
                r = "HTTP/1.1 407 Proxy Authentication Required";
                break;
            case Status::RequestTimeout:
                r = "HTTP/1.1 408 Request Time-out";
                break;
            case Status::Conflict:
                r = "HTTP/1.1 409 Conflict";
                break;
            case Status::Gone:
                r = "HTTP/1.1 410 Gone";
                break;
            case Status::LengthRequired:
                r = "HTTP/1.1 411 Length Required";
                break;
            case Status::PreconditionFailed:
                r = "HTTP/1.1 412 Precondition Failed";
                break;
            case Status::RequestEntityTooLarge:
                r = "HTTP/1.1 413 Request Entity Too Large";
                break;
            case Status::RequestUriTooLarge:
                r = "HTTP/1.1 414 Request-URI Too Large";
                break;
            case Status::UnsupportedMediaType:
                r = "HTTP/1.1 415 Unsupported Media Type";
                break;
            case Status::RequestRangeInvalid:
                r = "HTTP/1.1 416 Requested range not satisfiable";
                break;
            case Status::ExpectationFailed:
                r = "HTTP/1.1 417 Expectation Failed";
                break;
            case Status::InternalError:
                r = "HTTP/1.1 500 Internal Server Error";
                break;
            case Status::NotImplemented:
                r = "HTTP/1.1 501 Not Implemented";
                break;
            case Status::BadGateway:
                r = "HTTP/1.1 502 Bad Gateway";
                break;
            case Status::ServiceUnavailable:
                r = "HTTP/1.1 503 Service Unavailable";
                break;
            case Status::GatewayTimeout:
                r = "HTTP/1.1 504 Gateway Time-out";
                break;
            case Status::BadVersion:
                r = "HTTP/1.1 505 HTTP Version not supported";
                break;
            default:
                r = "HTTP/1.1 500  ";
                break;
        }
        return (r);
    }

    String toString(Method method)
    {
        switch(method)
        {
            case Method::Delete:
                return "DELETE";
            case Method::Get:
                return "GET";
            case Method::Head:
                return "HEAD";
            case Method::Post:
                return "POST";
            case Method::Put:
                return "PUT";
            case Method::Connect:
                return "CONNECT";
            case Method::Options:
                return "OPTIONS";
            case Method::Trace:
                return "TRACE";
            default:
                return "Invalid";
        }
    }

    Method fromString(const String& name)
    {
        switch (::toupper(name.empty()? '\0': name[0])) {
            case 'D':
                return Method::Delete;
            case 'G':
                return Method::Get;
            case 'H':
                return Method::Head;
            case 'P':
                if (::toupper(name[1]) == 'O')
                    return Method::Post;
                else
                    return Method::Put;
            case 'C':
                return Method::Connect;
            case 'O':
                return Method::Options;
            case 'T':
                return Method::Trace;
            default:
                return Method::Unknown;
        }
    }
}