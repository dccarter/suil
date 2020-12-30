//
// Created by Mpho Mbotho on 2020-12-29.
//

#ifndef SUIL_HTTP_CLIENT_RESPONSE_HPP
#define SUIL_HTTP_CLIENT_RESPONSE_HPP

#include <suil/http/parser.hpp>
#include <suil/http/common.hpp>
#include <suil/net/server.hpp>

namespace suil::http::cli {

    class Response: protected HttpParser {
    public:
        using Writer = std::function<bool(const char*, size_t)>;
        Response();

        MOVE_CTOR(Response);
        MOVE_ASSIGN(Response);

        inline http::Status status() const {
            return http::Status(status_code);
        }

        inline const String& redirect() const {
            return header("Location");
        }

        const String& header(const String& name) const;

        inline const UnorderedMap<String,CaseInsensitive>& headers() const {
            return _headers;
        }

        String str() const;

        Data data() const;

        inline operator bool() const {
            return status() == http::Ok;
        }

        const String& contentType() const {
            return header("Content-Type");
        }

        inline const Buffer& operator()() const {
            return _stage;
        }

        inline Buffer& operator()() {
            return _stage;
        }

    protected:
        int onBodyPart(const String &part) override;
        int onHeadersComplete() override;
        int onMessageComplete() override;

    private:
        DISABLE_COPY(Response);
        void receive(net::Socket& sock, int64 timeout = -1);
        friend class Session;
        bool _bodyRead{false};
        Writer _writer{nullptr};
    };
}
#endif //SUIL_HTTP_CLIENT_RESPONSE_HPP
