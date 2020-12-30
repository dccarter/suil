//
// Created by Mpho Mbotho on 2020-12-14.
//

#include "suil/http/parser.hpp"

namespace suil::http {

    int HttpParser::on_message_begin(llhttp_t* p)
    {
        auto hp = static_cast<HttpParser *>(p);
        hp->clear(true);
        return HPE_OK;
    }

    int HttpParser::on_url(llhttp_t *p, const char *at, size_t len)
    {
        auto hp = static_cast<HttpParser *>(p);
        hp->_stage.append(at, len);
        return HPE_OK;
    }

    int HttpParser::on_url_complete(llhttp_t *p)
    {
        auto hp = static_cast<HttpParser *>(p);
        return hp->onUrl(String{hp->_stage});
    }

    int HttpParser::on_status(llhttp_t *p, const char *at, size_t len)
    {
        auto hp = static_cast<HttpParser *>(p);
        hp->_stage.append(at, len);
        return HPE_OK;
    }

    int HttpParser::on_status_complete(llhttp_t *p)
    {
        auto hp = static_cast<HttpParser *>(p);
        return hp->onStatus(String{hp->_stage});
    }

    int HttpParser::on_header_field(llhttp_t *p, const char* at, size_t len)
    {
        auto hp = static_cast<HttpParser *>(p);
        hp->_stage.append(at, len);
#if 1 // @TODO hack see https://github.com/nodejs/llhttp/pull/81
        if (String{hp->_stage, false}.compare("Connection", true) == 0) {
            hp->_follow = String{hp->_stage};
        }

        if (p->type == HTTP_RESPONSE and String{hp->_stage, false}.compare("Content-Length", true) == 0) {
            hp->_follow = String{hp->_stage};
        }
#endif
        return HPE_OK;
    }

    int HttpParser::on_header_field_complete(llhttp_t *p)
    {
        auto hp = static_cast<HttpParser *>(p);
        hp->_follow = String{hp->_stage};
        return HPE_OK;
    }

    int HttpParser::on_header_value(llhttp_t *p, const char *at, size_t len)
    {
        auto hp = static_cast<HttpParser *>(p);
        hp->_stage.append(at, len);
        return HPE_OK;
    }

    int HttpParser::on_header_value_complete(llhttp_t *p)
    {
        auto hp = static_cast<HttpParser *>(p);
        hp->_headers.emplace(std::move(hp->_follow), String{hp->_stage});
        return HPE_OK;
    }

    int HttpParser::on_headers_complete(llhttp_t *p)
    {
        auto hp = static_cast<HttpParser *>(p);
        if (hp->_follow) {
            hp->_headers.emplace(String{hp->_follow}, String{hp->_stage});
        }
        hp->_headersComplete = 1;
        return hp->onHeadersComplete();
    }

    int HttpParser::on_body(llhttp_t *p, const char *at, size_t len)
    {
        auto hp = static_cast<HttpParser *>(p);
        return hp->onBodyPart(String{at, len, false});
    }

    int HttpParser::on_msg_complete(llhttp_t *p)
    {
        auto hp = static_cast<HttpParser *>(p);
        hp->_bodyComplete = 1;
        return hp->onMessageComplete();
    }

    int HttpParser::on_chunk_header(llhttp_t *p)
    {
        auto hp = static_cast<HttpParser *>(p);
        return hp->onChunkHeader();
    }

    int HttpParser::on_chunk_complete(llhttp_t *p)
    {
        auto hp = static_cast<HttpParser *>(p);
        return hp->onChunkComplete();
    }

    HttpParser::HttpParser(llhttp_type type)
    {
        const static llhttp_settings_t PARSER_SETTINGS {
                HttpParser::on_message_begin,
                HttpParser::on_url,
                HttpParser::on_status,
                HttpParser::on_header_field,
                HttpParser::on_header_value,
                HttpParser::on_headers_complete,
                HttpParser::on_body,
                HttpParser::on_msg_complete,
                HttpParser::on_chunk_header,
                HttpParser::on_chunk_complete,
                HttpParser::on_url_complete,
                HttpParser::on_status_complete,
                HttpParser::on_header_field_complete,
                HttpParser::on_header_value_complete
        };
        llhttp_init(this, type, &PARSER_SETTINGS);
    }

    bool HttpParser::feed(const char* data, size_t len)
    {
        auto err = llhttp_execute(this, data, len);
        if (err != HPE_OK) {
            if (err == HPE_PAUSED_UPGRADE) {
                llhttp_resume_after_upgrade(this);
                return true;
            }
            return false;
        }

        return true;
    }

    void HttpParser::clear(bool intern)
    {
        _follow = {};
        if (!intern) {
            _stage.clear();
        }
        Ego._headersComplete = 0;
        Ego._bodyComplete = 0;
        Ego._headers.clear();
    }

    int HttpParser::onBodyPart(const String& part)
    {
        Ego._stage << part;
        return HPE_OK;
    }
}