//
// Created by Mpho Mbotho on 2020-12-14.
//

#ifndef SUIL_HTTP_SERVER_COMMON_HPP
#define SUIL_HTTP_SERVER_COMMON_HPP

#include <suil/base/string.hpp>
#include <suil/base/exception.hpp>

namespace suil::http {

    enum Status : int {
        Unknown                     = 0,
        Continue			        = 100,
        SwitchingProtocols		    = 101,
        Ok				            = 200,
        Created			            = 201,
        Accepted			        = 202,
        NonAuthoritative		    = 203,
        NoContent			        = 204,
        ResetContent		        = 205,
        PartialContent		        = 206,
        MultipleChoices		        = 300,
        MovedPermanently		    = 301,
        Found			            = 302,
        SeeOther			        = 303,
        NotModified		            = 304,
        UseProxy			        = 305,
        TemporaryRedirect		    = 307,
        BadRequest			        = 400,
        Unauthorized		        = 401,
        PaymentRequired		        = 402,
        Forbidden			        = 403,
        NotFound			        = 404,
        MethodNotAllowed		    = 405,
        NotAcceptable		        = 406,
        ProxyAuthRequired		    = 407,
        RequestTimeout		        = 408,
        Conflict			        = 409,
        Gone			            = 410,
        LengthRequired		        = 411,
        PreconditionFailed		    = 412,
        RequestEntityTooLarge	    = 413,
        RequestUriTooLarge	        = 414,
        UnsupportedMediaType	    = 415,
        RequestRangeInvalid	        = 416,
        ExpectationFailed		    = 417,
        InternalError		        = 500,
        NotImplemented		        = 501,
        BadGateway			        = 502,
        ServiceUnavailable	    	= 503,
        GatewayTimeout		        = 504,
        BadVersion			        = 505
    };

    DECLARE_EXCEPTION(HttpException);

    String toString(Status status);

    class HttpError : public HttpException {
    public:
        template <typename ...Args>
        HttpError(Status code, Args&&... args)
                : HttpException(std::forward<Args>(args)...),
                  Code{code}
        {}

        inline String statusText() const { return toString(Code); }
        Status Code{Status::Ok};
    };

    enum class Method : unsigned char {
        Delete = 0,
        Get,
        Head,
        Post,
        Put,
        Connect,
        Options,
        Trace,
        Unknown
    };

    suil::String toString(Method method);

    Method fromString(const String& name);

}

#endif //SUIL_HTTP_SERVER_COMMON_HPP
