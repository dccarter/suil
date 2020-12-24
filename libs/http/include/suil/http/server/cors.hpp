//
// Created by Mpho Mbotho on 2020-12-19.
//

#ifndef SUIL_HTTP_SERVER_CORS_HPP
#define SUIL_HTTP_SERVER_CORS_HPP

#include <suil/http/server.scc.hpp>
#include <suil/base/sio.hpp>
#include <suil/base/string.hpp>

namespace suil::http::server {

    class Request;
    class Response;

    class Cors {
    public:
        struct Context{
        };

        void before(Request& req, Response&, Context&);

        void after(Request&, Response&, Context&);

        template<typename __T>
        void configure(__T& opts) {
            if (opts.has(sym(allowOrigin))) {
                Ego._allowOrigin = opts.has(sym(allowOrigin), "");
            }

            if (opts.has(sym(allowHeaders))) {
                Ego._allowHeaders = opts.has(sym(allowHeaders), "");
            }
        }

        template <typename E, typename...__Opts>
        void setup(E& ep, __Opts... args) {
            auto opts = iod::D(args...);
            configure(opts);
        }

    private:
        suil::String   _allowOrigin{"*"};
        suil::String   _allowHeaders{"Origin, X-Requested-With, Content-Type, Accept, Authorization"};
    };

}
#endif //SUIL_HTTP_SERVER_CORS_HPP
