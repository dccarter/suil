//
// Created by Mpho Mbotho on 2020-12-15.
//

#ifndef SUIL_HTTP_SERVER_REQUEST_HPP
#define SUIL_HTTP_SERVER_REQUEST_HPP

#include <suil/http/server.scc.hpp>
#include <suil/http/route.scc.hpp>
#include <suil/http/cookie.hpp>
#include <suil/http/common.hpp>
#include <suil/http/offload.hpp>
#include <suil/http/parser.hpp>
#include <suil/http/server/form.hpp>
#include <suil/http/server/qs.hpp>
#include <suil/http/server/routes.hpp>

#include <suil/base/file.hpp>
#include <suil/base/logging.hpp>
#include <suil/net/socket.hpp>

namespace suil::http::server {

    define_log_tag(HTTP_REQ);

    class Request : public HttpParser, public LOGGER(HTTP_REQ) {
    public:
        const char *ip() const;
        int port() const;
        net::Socket& sock();
        strview body() const;
        size_t readBody(void *buf, size_t len);
        bool isValid() const;
        const String& header(const String& name) const;
        void header(String name, String value);

        inline const UnorderedMap<String>& cookies() const {
            return Ego._cookies;
        }

        const String& cookie(const String& name) const;

        inline const UnorderedMap<String, CaseInsensitive>& headers() const {
            return Ego._headers;
        }

        inline const Form& form() const {
            return Ego._form;
        }

        inline const QueryString& query() const {
            return Ego._qps;
        }

        inline const String& url() const {
            return Ego._url;
        }

        template <typename T>
        inline void toJson(T& o) const {
            if (!Ego._flags.bodyOffload) {
                json::decode(Ego._stage, o);
            }
            else {
                auto data = Ego._offload.data();
                json::decode(data, o);
            }
        }

        inline const RequestParams& params() const {
            return Ego._params;
        }

        const RouteAttributes& attrs() const;

        inline bool enabled() const {
            return attrs().Enabled;
        }

        inline void enabled(bool on) {
            Ego._params.attrs->Enabled = on;
        }

        inline int32 routeId() const {
            return Ego._params.routeId;
        }

        inline Method getMethod() const {
            return Method(Ego.method);
        }

        void clear(bool internal = false) override;

        void *middlewareContent{nullptr};

    protected:
        int onBodyPart(const String &part) override;
        int onUrl(String &&url) override;
        int onHeadersComplete() override;
        int onMessageComplete() override;

    private suil_ut:
        Request(net::Socket& sock, HttpServerConfig& config);
        bool parseCookies();
        bool parseForm();
        bool parseUrlEncodedForm();
        bool parseMultipartForm(const String& boundary);
        Status receiveHeaders(HttpServerStats& stats);
        Status receiveBody(HttpServerStats& stats);
        Data readBody();
        template <typename...Args>
        bool anyMethod(Method m, Args... args) {
            if constexpr (sizeof...(args) > 0) {
                return ((uint8) m == Ego.method) or Ego.anyMethod(args...);
            }
            return (uint8) m == Ego.method;
        }


        struct {
            uint8 hasBody      : 1;
            uint8 bodyError    : 1;
            uint8 offloadError : 1;
            uint8 formPassed   : 1;
            uint8 cookiesParsed: 1;
            uint8 bodyOffload  : 1;
            uint8 u8           : 2;
        } _flags = {0};

        static uint64 sOffloadIndex;
        uint32 _bodyOffset{0};
        Status _status{http::Ok};
        Form   _form{};
        FileOffload _offload;
        UnorderedMap<String> _cookies{};
        String _url;
        QueryString _qps;
        net::Socket& _sock;
        HttpServerConfig& _config;

        friend class SystemAttrs;
        friend class Router;
        friend class ConnectionImpl;
        RequestParams _params{};
    };
}
#endif //SUIL_HTTP_SERVER_REQUEST_HPP
