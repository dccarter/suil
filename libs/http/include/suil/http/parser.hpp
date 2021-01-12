//
// Created by Mpho Mbotho on 2020-12-14.
//

#ifndef LIBS_HTTP_SRC_PARSER_HPP
#define LIBS_HTTP_SRC_PARSER_HPP

#include <suil/http/llhttp.h>

#include <suil/base/buffer.hpp>

namespace suil::http {

    class HttpParser : protected llhttp_t {
    protected suil_ut:
        HttpParser(llhttp_type type = HTTP_REQUEST);
        bool feed(const char *data, size_t len);
        virtual void clear(bool intern = false);

        inline bool isUpgrade() const {
            return upgrade != 0;
        }

        inline bool isVersion(int major, int minor) const {
            return (http_major == major) and (http_minor == minor);
        }

        inline bool done() {
            return feed(nullptr, 0);
        }

        virtual int onHeadersComplete() { return HPE_OK; }
        virtual int onBodyPart(const String& part);
        virtual int onMessageComplete() { return HPE_OK; }
        virtual int onStatus(String&& status) { return HPE_OK; }
        virtual int onUrl(String&& url) { return HPE_OK; }
        virtual int onChunkHeader()   { return HPE_OK; };
        virtual int onChunkComplete() { return HPE_OK; };

        struct {
            uint8_t _headersComplete : 1 = 0;
            uint8_t _bodyComplete    : 1 = 0;
            uint8_t _u6 : 6              = 0;
        } __attribute__((packed));

        UnorderedMap<String, CaseInsensitive> _headers{};
        Buffer      _stage{0};
        String      _follow;

    private:
        static int on_headers_complete(llhttp_t *);
        static int on_chunk_header(llhttp_t *);
        static int on_chunk_complete(llhttp_t *);
        static int on_url(llhttp_t *, const char *, size_t);
        static int on_url_complete(llhttp_t *);
        static int on_status(llhttp_t *, const char *, size_t);
        static int on_status_complete(llhttp_t *);
        static int on_body(llhttp_t *, const char *, size_t);
        static int on_msg_complete(llhttp_t *);
        static int on_header_field(llhttp_t *, const char *, size_t);
        static int on_header_field_complete(llhttp_t *);
        static int on_header_value(llhttp_t *, const char *, size_t);
        static int on_header_value_complete(llhttp_t *);
        static int on_message_begin(llhttp_t *);
    };
}
#endif //SUIL_HTTP_SERVER_PARSER_HPP
