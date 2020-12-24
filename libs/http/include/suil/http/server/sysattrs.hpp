//
// Created by Mpho Mbotho on 2020-12-19.
//

#ifndef SUIL_HTTP_SERVER_SYSATTRS_HPP
#define SUIL_HTTP_SERVER_SYSATTRS_HPP

#include <suil/http/server.scc.hpp>
#include <suil/base/sio.hpp>

namespace suil::http::server {

    class Request;
    class Response;

    class SystemAttrs {
    public:
        struct Context{
        };

        void before(Request& req, Response&, Context&);

        void after(Request&, Response&, Context&);

        template<typename T>
        void configure(T& opts) {
            cookies = opts.get(sym(cookies), false);
        }

        template <typename... Opts>
        void setup(Opts... args) {
            auto opts = iod::D(args...);
            configure(opts);
        }

    private:
        bool cookies{false};
    };
}
#endif //SUIL_HTTP_SERVER_SYSATTRS_HPP
