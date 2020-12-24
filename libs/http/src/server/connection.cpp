//
// Created by Mpho Mbotho on 2020-12-18.
//

#include "suil/http/server/connection.hpp"

#include <suil/base/datetime.hpp>

namespace suil::http::server {

    ConnectionImpl::ConnectionImpl(
            net::Socket& sock,
            HttpServerConfig& config,
            Router& handler,
            HttpServerStats& stats)
        : _config{config},
          _stats{stats},
          _sock{sock},
          _handler{handler}
    {
        Ego._stats.totalRequests++;
        Ego._stats.openRequests++;
    }

    ConnectionImpl::~ConnectionImpl() noexcept
    {
        Ego._stats.openRequests--;
    }

    void ConnectionImpl::start()
    {
        Status status = http::Ok;
        Request req(Ego._sock, Ego._config);

        itrace("% - starting connection handler", Ego._sock.id());
        do {
            // receive request headers
            status = req.receiveHeaders(Ego._stats);
            if (status != http::Ok) {
                if (status != http::RequestTimeout) {
                    // receiving failed, send back failure
                    Response resp{status};
                    sendResponse(req, resp, true);
                }
                else {
                    // receiving timed out, abort connection
                    itrace("%s - receiving headers timed out", Ego._sock.id());
                }
                break;
            }

            // receive request body
            status = req.receiveBody(Ego._stats);
            if (status != http::Ok) {
                if (status != http::RequestTimeout) {
                    // receiving failed, send back failure
                    Response resp{status};
                    sendResponse(req, resp, true);
                }
                else {
                    // receiving timed out, abort connection
                    itrace("%s - receiving body timed out", Ego._sock.id());
                }
                break;
            }

            // handle received request
            bool err{false};
            int64_t start = mnow();
            Response resp{http::Ok};
            try {
                handleRequest(req, resp);
            }
            catch(HttpError& ex) {
                err = true;
                resp._status = ex.Code;
                resp._body.reset(0, true);
                resp._chunks.clear();
                resp.append(ex.statusText());
            }
            catch (...) {
                auto ex = Exception::fromCurrent();
                err = true;
                resp._status = http::InternalError;
                resp._chunks.clear();
                resp._body.reset(0, true);
                idebug("%s - Request handle unhandled error: %s", Ego._sock.id(), ex.what());
            }
            sendResponse(req, resp, err);
            if (resp._status == http::SwitchingProtocols) {
                // easily switch protocols
                resp()(req, resp);
                Ego._close = true;
            }

            itrace("\"%s " PRIs " HTTP/%u.%u\" %u - %lu ms",
                   http::toString(Method(req.method))(), _PRIs(req.url()),
                   req.http_major, req.http_minor, resp._status, (mnow()-start));

            req.clear();
            resp.clear();
        } while (!Ego._close);

        itrace("%s - done handling Connection, %d", Ego._sock.id(), _close);
    }

    void ConnectionImpl::sendResponse(Request& req, Response& resp, bool err)
    {
        if (!Ego._sock.isOpen()) {
            Ego._close = true;
            itrace("%s send response failed: socket already closed", Ego._sock.id());
            return;
        }
        SendBuffer sb;
        sb.reserve(resp._headers.size() - resp._chunks.size() + 5);
        Ego._stage.reset(1024, true);

        auto status = http::toString(resp._status);
        Ego._stage << status;
        Ego._stage.append("\r\n", 2);

        if (!err) {
            const auto& conn = req.header("Connection");
            if (conn.compare("Close", true) == 0) {
                // client requested Connection be closed
                Ego._close = true;
            }

            if (Ego._config.keepAliveTime and !Ego._close) {
                // set keep alive time
                constexpr strview keepAlive{"Connection: Keep-Alive\r\n"};
                Ego._stage.append(keepAlive);
                Ego._stage.appendnf(30, "KeepAlive: %lu\r\n", Ego._config.keepAliveTime);
            }

            if (Ego._config.hstsEnable) {
                Ego._stage.reserve(48);
                Ego._stage << "Strict-Transport-Security: max-age "
                           << Ego._config.hstsEnable << "; includeSubdomains\r\n";
            }
        }
        else {
            // force connection close on error
            Ego._close = true;
        }

        if ((req._status > http::BadRequest) and resp.size() == 0) {
            resp.append(status.substr(9));
        }

        resp.flushCookies();
        for (const auto& [k, v]: resp._headers) {
            Ego._stage.append(k.data(), k.size());
            Ego._stage.append(": ", sizeofcstr(": "));
            Ego._stage.append(v.data(), v.size());
            Ego._stage.append("\r\n", 2);
        }

        if (!resp._headers.contains("Server")) {
            Ego._stage.append("Server: ", sizeofcstr("Server: "));
            Ego._stage << Ego._config.serverName;
            Ego._stage.append("\r\n", 2);
        }

        if (!resp._headers.contains("Date")) {
            Ego._stage.append("Date: ", sizeofcstr("Date: "));
            Ego._stage.append(getCachedDate());
            Ego._stage.append("\r\n", 2);
        }

        if (!resp._headers.contains("Content-Length")) {
            Ego._stage.append("Content-Length: ", sizeofcstr("Content-Length: "));
            auto tmp = std::to_string(resp.size());
            Ego._stage.append(tmp.data(), tmp.size());
            Ego._stage.append("\r\n", 2);
        }

        if (!resp._headers.contains("Content-Type")) {
            Ego._stage.append("Content-Type: ", sizeofcstr("Content-Type: "));
            if (req._params.attrs and req.attrs().ReplyType) {
                Ego._stage << req.attrs().ReplyType;
            }
            else {
                // default content type "text/plain"
                Ego._stage.append("text/plain", sizeofcstr("text/plain"));
            }
            Ego._stage.append("\r\n", 2);
        }

        Ego._stage.append("\r\n", 2);
        sb.emplace_back(Ego._stage.data(), Ego._stage.size());
        if (!resp._chunks.empty()) {
            for(auto& ch: resp._chunks) {
                // move chunks from response to send buffer
                sb.push_back(std::move(ch));
            }
        }

        if (!resp._body.empty()) {
            sb.emplace_back(resp._body.data(), resp._body.size());
        }

        if (!Ego.writeResponse(sb)) {
            iwarn("%s sending data to socked failed: %s", Ego._sock.id(), errno_s);
            Ego._close = true;
            resp.clear();
        }
        sb.clear();
    }

    bool ConnectionImpl::writeResponse(const SendBuffer& buf)
    {
        size_t rc{0};
        for (auto& b: buf) {
            if (b.usesFd()) {
                // send file descriptor
                size_t sent{0};
                do {
                    auto chunk = std::min(Ego._config.sendChunk, b.size() - sent);
                    rc = Ego._sock.sendfile(
                            b.fd(),
                            (b.offset() + sent),
                            chunk,
                            Ego._config.connectionTimeout);
                    if (rc == 0 || rc != chunk) {
                        itrace("%s sending failed: %s", Ego._sock.id(), errno_s);
                        return false;
                    }

                    sent += chunk;
                } while (sent < b.size());
            }
            else {
                // send buffer
                size_t sent{0};
                auto data = static_cast<const char*>(b.ptr());
                do {
                    auto chunk = std::min(Ego._config.sendChunk, b.size()-sent);
                    rc = Ego._sock.send(&data[sent], chunk, Ego._config.connectionTimeout);
                    if (rc == 0 || rc != chunk) {
                        itrace("%s sending failed: %s", Ego._sock.id(), errno_s);
                        return false;
                    }

                    sent += chunk;
                } while (sent < b.size());
            }
            Ego._stats.txBytes += b.size();
        }

        return Ego._sock.flush(Ego._config.connectionTimeout);
    }

    const char* ConnectionImpl::getCachedDate()
    {
        static uint64 rec{0};
        auto point = mnow();
        static char buf[64] = {0};
        if ((point - rec) > 1000) {
            // update timestamp after every second
            rec = point;
            Datetime{}(buf, sizeof(buf), Datetime::HTTP_FMT);
        }

        return buf;
    }

}