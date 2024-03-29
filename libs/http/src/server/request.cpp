//
// Created by Mpho Mbotho on 2020-12-15.
//

#include "suil/http/server/request.hpp"

#include <suil/base/url.hpp>

namespace suil::http::server {

    uint64 Request::sOffloadIndex{0};

    Request::Request(net::Socket& sock, HttpServerConfig& config)
        : _sock{sock},
          _config{config}
    {}

    const char* Request::ip() const
    {
        return net::Socket::ipstr(Ego._sock.addr());
    }

    int Request::port() const {
        return Ego._sock.port();
    }

    net::Socket& Request::sock() {
        return Ego._sock;
    }

    const String& Request::header(const String& name) const
    {
        auto it = Ego._headers.find(name);
        if (it == Ego._headers.end()) {
            static String Invalid{};
            return Invalid;
        }
        return it->second;
    }

    const String & Request::cookie(const String& name) const {
        auto it = Ego._cookies.find(name);
        if (it == Ego._headers.end()) {
            static String Invalid{};
            return Invalid;
        }
        return it->second;
    }

    void Request::header(String name, String value)
    {
        Ego._headers.emplace(std::move(name), std::move(value));
    }

    bool Request::isValid() const {
        return !Ego._flags.bodyError and !Ego._flags.offloadError;
    }

    strview Request::body() const
    {
        if (!Ego.isValid()) {
            return {};
        }

        if (Ego._flags.bodyOffload) {
            return Ego._offload.data();
        }
        else {
            return {Ego._stage.data(), Ego._stage.size()};
        }
    }

    const RouteAttributes& Request::attrs() const {
        if (Ego.params().attrs == nullptr) {
            throw HttpError(http::InternalError, "Route missing attributes");
        }
        return *Ego.params().attrs;
    }

    void Request::clear(bool internal)
    {
        HttpParser::clear(internal);
        Ego._form.clear();
        Ego._cookies.clear();
        Ego._offload.close();
        Ego._url = {};
        Ego._qps.clear();
        Ego._bodyOffset = 0;
        memset(&Ego._flags, 0, sizeof(Ego._flags));
        Ego._params.clear();
    }

    int Request::onUrl(String&& url)
    {
        auto pos = url.find('?');
        if (pos != -1) {
            Ego._qps = QueryString{url.substr(pos)};
            Ego._url = url.substr(0, pos, false);
        }
        else {
            Ego._url = std::move(url);
        }

        return 0;
    }

    bool Request::parseCookies()
    {
        if (Ego._flags.cookiesParsed) {
            return true;
        }

        Ego._flags.cookiesParsed = 1;

        auto it = Ego._headers.find("Cookie");
        if (it == Ego._headers.end()) {
            // no cookies to parse
            return false;
        }
        // this will modify the cookie string
        auto parts = it->second.split(";");
        for (auto& part: parts) {
            size_t i{0};
            while (i < part.size() and isspace(part[i])) i++;
            if (i == part.size()) {
                itrace("invalid cookie in header");
                continue;
            }
            auto pos = part.find('=');
            if (pos == String::npos) {
                // cookie has no value
                Ego._cookies.emplace(part.substr(i), String{});
            }
            else {
                // cookie has value
                Ego._cookies.emplace(part.substr(i, pos), part.substr(pos+1));
            }
        }

        return true;
    }

    bool Request::parseForm()
    {
        if (Ego._flags.formPassed) {
            itrace("form parse already attempted");
            return true;
        }

        Ego._flags.formPassed = 1;

        if (!anyMethod(Method::Post, Method::Put)) {
            itrace("parsing for in unexpect method " PRIs, _PRIs(toString((Method) method)));
            return false;
        }

        auto ctype = header("Content-Type");
        if (!ctype) {
            itrace("only requests with content type are supported");
            return false;
        }

        if (ctype == "application/x-www-form-urlencoded") {
            itrace("Request::parseForm parsing url encoded form");
            return parseUrlEncodedForm();
        }

        if (ctype.compare("multipart/form-data", true) == 0) {
            itrace("Request::parseForm parsing multipart form");
            auto pos = ctype.find('=');
            if (pos == String::npos) {
                idebug("Request::parseForm multipart/form-data without boundary " PRIs, _PRIs(ctype));
                return false;
            }
            pos++;
            return Ego.parseMultipartForm(ctype.substr(pos));
        }

        idebug("Request::parseForm content type " PRIs " cannot be parsed as a form", _PRIs(ctype));
        return false;
    }

#define eat_space(p, i) while (isspace((p)[i]) && (p)[i] != '\r') i++

    static inline bool _get_header(const String& p, int &idx, String& f, String& v) {
        auto tmp = p.substr(idx);
        auto pos = tmp.find(':');
        if (pos == String::npos) {
            return false;
        }

        f = tmp.substr(0, pos++);
        eat_space(tmp, pos);
        tmp = tmp.substr(pos);
        pos = 0;
        while ((tmp[++pos] != '\r') and (pos < tmp.size()));

        if ((pos < tmp.size()) and (tmp[++pos] == '\n')) {
            idx = pos;
            v = tmp.substr(0, pos-2);
            return true;
        }
        idx = pos;
        return false;
    }

#define goto_chr(p, i, c) ({bool __v = false;                       \
        while (i < (p).size() && (p)[i] != (c) && (p)[i] != '\r') i++;    \
        if ((p)[i] == (c)) __v = true; __v; })

    static inline bool _get_disposition(String& p, int& idx, String& name, String& filename) {
        int n = 0, f = 0;
        while (p[idx] != '\0' && p < p.size()) {
            if (strncasecmp(&p[idx], "name=\"", 6) == 0) {
                /* eat name=" */
                idx += 6;
                auto tmp = p.substr(idx);
                if (goto_chr(tmp, n, '"')) {
                    name = tmp.substr(0, n);
                    idx += n;
                } else {
                    return false;
                }
            }
            else if (strncasecmp(&p[idx], "filename=\"", 10) == 0) {
                /* eat filename= */
                idx += 10;
                auto tmp = p.substr(idx);
                if (goto_chr(p, f, '"')) {
                    filename = tmp.substr(0, f);
                    idx += n;
                } else {
                    return false;
                }
            }
            else {
                /* unknown field */
                return false;
            }

            if (p[idx] == ';') idx++;
            eat_space(p, idx);
        }

        return true;
    }

    bool Request::parseMultipartForm(const String& boundary) {
        auto body = readBody();
        if (body.empty()) {
            idebug("Request::parseMultipartForm: nothing to parse, body empty");
            return false;
        }

        enum {
            state_begin, state_is_boundary,
            state_boundary, state_save_data,
            state_save_file, state_header,
            state_content, state_data,
            state_error, state_end
        }  state = state_begin, next_state = state_begin;

        int idx{0};
        auto p = String{reinterpret_cast<const char *>(body.data()), body.size(), false};
        int ds{0};
        String name;
        String filename;
        String pay;

        bool cap{false};
        itrace("start parse multipart form %d", mnow());
        while (cap || (idx < p.size())) {
            switch (next_state) {
                case state_begin:
                    itrace("multipart/form-data state_begin");
                    /* fall through */
                    state = state_error;
                    next_state = state_is_boundary;
                case state_is_boundary:
                    itrace("multipart/form-data state_is_boundary");
                    if (p[idx] == '-' && p[++idx] == '-') {
                        state = state_is_boundary;
                        next_state = state_boundary;
                        ++idx;
                    } else {
                        next_state = state;
                    }
                    break;

                case state_boundary:
                    state = state_is_boundary;
                    itrace("multipart/form-data state_boundary");
                    if (strncmp(&p[idx], boundary.data(), boundary.size()) != 0) {
                        /* invalid boundary */
                        itrace("error: multipart/form-data invalid boundary");
                        next_state = state_error;
                    }
                    else {
                        idx += boundary.size();
                        bool el = (p[idx] == '\r' && p[idx+1] == '\n');
                        bool eb = (p[idx] == '-' && p[idx+1] == '-');
                        if (el || eb)
                        {
                            state = el? state_content : state_end;
                            idx += 2;
                            if (filename != nullptr) {
                                next_state = state_save_file;
                                cap = true;
                            } else if (name != nullptr) {
                                next_state = state_save_data;
                                cap = true;
                            } else if (el) {
                                next_state = state_content;
                            } else
                                next_state = state_end;
                        }
                        else {
                            itrace("error: multipart/form-data invalid state %d %d", el, eb);
                            next_state = state_error;
                        }
                    }
                    break;

                case state_save_file: {
                    itrace("multipart/form-data state_save_file");
                    UploadedFile f{std::move(filename), Data{pay.data(), pay.size(), false}};
                    Ego._form.add(std::move(name), std::move(f));
                    filename = {};
                    pay = {};
                    name = {};
                    next_state = state;
                    cap = false;

                    break;
                }

                case state_save_data: {
                    itrace("multipart/form-data state_save_data");
                    Ego._form.add(std::move(name), std::move(pay));
                    name = {};
                    pay = {};
                    next_state = state;
                    cap = false;
                    break;
                }

                case state_content: {
                    itrace("multipart/form-data state_content");
                    String dp{}, val{};
                    state = state_content;
                    next_state = state_error;
                    if (_get_header(p, idx, dp, val) &&
                        dp.compare("Content-Disposition", true) == 0) {
                        idx++;
                        itrace("multipart/form-data content disposition: " PRIs, _PRIs(val));
                        if (!val.startsWith("form-data", true) != 0) {
                            /* unsupported disposition */
                            itrace("error: multipart/form-data not content disposition: " PRIs,
                                   _PRIs(val));
                            continue;
                        }
                        /* eat form-data */
                        int vidx = 10;
                        eat_space(val, vidx);
                        if (_get_disposition(val, vidx, name, filename)) {
                            /* successfully go disposition */
                            itrace("multipart/form-data name: " PRIs ", filename: " PRIs,
                                   _PRIs(name), _PRIs(filename));
                            next_state = state_header;
                        } else {
                            itrace("error: multipart/form-data invalid disposition: " PRIs, _PRIs(val));
                        }
                    } else {
                        itrace("error: multipart/form-data missing disposition");
                    }
                    break;
                }
                case state_header: {
                    state = state_header;
                    itrace("multipart/form-data state_header\n" PRIs,
                           &p[idx], std::min(size_t(100), p.size() - idx));
                    String field, value;
                    if (p[idx] == '\r' && p[++idx] == '\n') {
                        ds = ++idx;
                        next_state = state_data;
                    } else if (_get_header(p, idx, field, value)) {
                        idx++;
                        itrace("multipart/form-data header field: " PRIs ", value: " PRIs,
                               _PRIs(field), _PRIs(value));
                    } else {
                        itrace("error: multipart/form-data parsing header failed");
                        next_state = state_error;
                    }
                    break;
                }
                case state_data: {
                    state = state_data;
                    itrace("multipart/form-data state_data");
                    pay = p.substr(ds, idx);
                    idx++;
                    next_state = state_is_boundary;
                    break;
                }
                case state_end: {
                    state = state_end;
                    itrace("multipart/form-data state machine done %d fields, %d files %lld",
                           Ego._form._params.size(),
                           Ego._form._uploads.size(),
                           mnow());
                    return true;
                }
                case state_error:
                default:
                    itrace("error: multipart/form-data error in state machine");
                    return false;
            }
        }

        return false;
    }

#undef eat_space
#undef goto_chr

    bool Request::parseUrlEncodedForm() {
        auto data = Ego.readBody();
        if (data.empty() > 0) {
            String tmp{reinterpret_cast<char*>(data.data()), data.size(), false};
            auto parts = tmp.parts("&");
            for(auto& part : parts) {
                /* save all parameters in part */
                auto pos = part.find('=');
                String value;
                String name;
                if (pos != String::npos) {
                    name = part.substr(0, pos++);
                    value = part.substr(pos);
                }
                else {
                    name = part.peek();
                }

                Ego._form.add(std::move(name), URL::decode(value));
            }
            return true;
        }

        return false;
    }

    Status Request::receiveHeaders(HttpServerStats& stats)
    {
        Ego._status = http::Ok;
        char stage[2048];
        do {
            auto len = sizeof(stage);
            if (!sock().read(stage, len, Ego._config.connectionTimeout)) {
                if (errno) {
                    idebug("Request::receiveHeaders(%s) receiving headers failed: %s",
                           sock().id(), errno_s);
                }
                Ego._status = (errno == ETIMEDOUT) ? http::RequestTimeout : http::InternalError;
                break;
            }
            stats.rxBytes += len;

            if (!Ego.feed(stage, len)) {
                if (Ego._status == http::Ok)
                    Ego._status = http::BadRequest;
                break;
            }
        } while (Ego._headersComplete == 0);

        return Ego._status;
    }

    Status Request::receiveBody(HttpServerStats& stats)
    {
        if (Ego._bodyComplete) {
            return Ego._status;
        }

        char stage[8192];
        size_t len = 0, left = content_length;
        do {
            len = std::min(sizeof(stage), left);
            if (!sock().receive(stage, len, Ego._config.connectionTimeout)) {
                itrace("Request::receiveBody(%s) receive body failed: %s",
                       sock().id(), errno_s);
                Ego._status = (errno == ETIMEDOUT)? http::RequestTimeout : http::InternalError;
                Ego._flags.bodyError = 1;
                break;
            }
            stats.rxBytes += len;

            if (!feed(stage, len)) {
                if (Ego._status == http::Ok)
                    Ego._status = http::BadRequest;
                Ego._flags.bodyError = 1;
                break;
            }

            left -= len;
        } while (Ego._bodyComplete == 0);

        if ((Ego._status == http::Ok) and (Ego._bodyComplete == 0)) {
            itrace("Request::receiveBody(%s) parsing failed, body incomplete",
                   sock().id());
            Ego._flags.bodyError = 1;
            Ego._status = http::BadRequest;
        }

        return Ego._status;
    }

    int Request::onBodyPart(const String& part)
    {
        if (Ego._flags.bodyOffload and Ego._offload.valid()) {
            // offload body to file
            auto nwr = Ego._offload.write(part.data(), part.size(), Ego._config.connectionTimeout);
            if (nwr != part.size()) {
                idebug("Request::onBodyPart(%s) offload %zu bytes failed: %s",
                       sock().id(), part.size(), errno_s);
                Ego._flags.offloadError = 1;
                return -HPE_INTERNAL;
            }
        }
        else {
            // append received part to buffer
            return HttpParser::onBodyPart(part);
        }

        return HPE_OK;
    }

    size_t Request::readBody(void* buf, size_t len)
    {
        if (buf == nullptr or len == 0) {
            return 0;
        }

        if (!Ego._flags.hasBody) {
            // request does not have body to read
            return 0;
        }

        if (Ego._flags.bodyOffload) {
            auto data = Ego._offload.data();
            if (data.empty()) {
                idebug("Request::readBody(%s) getting offloaded data failed: %s",
                       sock().id(), errno_s);
                return 0;
            }

            auto maxRead = std::min(len, data.size()-Ego._bodyOffset);
            memcpy(buf, &data.data()[Ego._bodyOffset], maxRead);
            Ego._bodyOffset += maxRead;
            return maxRead;
        }
        else {
            // just copy from buffer
            auto maxRead = std::min(len, Ego._stage.size() - Ego._bodyOffset);
            memcpy(buf, &Ego._stage.data()[Ego._bodyOffset], maxRead);
            Ego._bodyOffset += maxRead;
            return maxRead;
        }
    }

    Data Request::readBody()
    {
        if (!Ego._flags.hasBody or Ego._flags.bodyError or Ego._flags.offloadError) {
            return Data{};
        }
        else if (Ego._flags.bodyOffload) {
            // this will map the file to memory and returned mapped buffer
            auto data = Ego._offload.data();
            if (data.empty()) {
                Ego._flags.offloadError = 1;
            }
            return data;
        }
        else {
            return Ego._stage.cdata();
        }
    }

    int Request::onMessageComplete()
    {
        Ego._flags.hasBody  = Ego.content_length != 0;
        return HttpParser::onMessageComplete();
    }

    int Request::onHeadersComplete()
    {
        if (Ego._headers.contains("Content-Length")) {
            if (Ego.content_length > _config.maxBodyLen) {
                idebug("Request::processHeaders(%s) request to large: %d",
                       sock().id(), content_length);
                Ego._status = http::RequestEntityTooLarge;
                return -HPE_INTERNAL;
            }
            Ego._flags.hasBody = 1;
        }

        if (Ego._flags.hasBody and Ego._config.diskOffload and
            (Ego.content_length > Ego._config.diskOffloadMin))
        {
            auto path = suil::catstr(Ego._config.offloadDir, "/http_body.", Ego.sOffloadIndex++);
            if (!Ego._offload.open(path, O_CREAT|O_RDWR, 0755)) {
                idebug("Request::processHeaders(%s) opening offload path %s failed: %s",
                       sock().id(), path(), errno_s);
                Ego._status = http::InternalError;
                Ego._flags.offloadError = 1;
                return -HPE_INTERNAL;
            }
            Ego._flags.bodyOffload = 1;
        }

        return HPE_OK;
    }
}