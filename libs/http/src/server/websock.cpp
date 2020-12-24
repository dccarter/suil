//
// Created by Mpho Mbotho on 2020-12-20.
//

#include "suil/http/server/websock.hpp"

#include <suil/base/base64.hpp>
#include <suil/base/hash.hpp>
#include <suil/base/uuid.hpp>

#include <climits>

#define WS_FRAME_HDR		2
#define WS_MASK_LEN		    4
#define WS_FRAME_MAXLEN		16384
#define WS_PAYLOAD_SINGLE	125
#define WS_PAYLOAD_EXTEND_1	126
#define WS_PAYLOAD_EXTEND_2	127
#define WS_OPCODE_MASK		0x0f

namespace {
    uint8_t sApiIndex{0};
    std::unordered_map<uint8_t, suil::http::WebSockApi&> sApis{};
    const suil::strview WsServerResponse{"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"};
}

namespace suil::http {

    WebSockApi::WebSockApi(int64 timeout, bool blockingBroadcast)
        : _timeout{timeout},
          _blockingBroadcast{blockingBroadcast}
    {
        Ego._id = sApiIndex++;
        sApis.emplace(Ego._id, *this);
    }

    WebSock* WebSockApi::find(const String& uuid)
    {
        auto it = Ego._webSocks.find(uuid);
        if (it != Ego._webSocks.end()) {
            return &it->second.get();
        }
        return nullptr;
    }

    void WebSockApi::send(chan ch, WebSock& ws, const void* data, size_t sz, uint8 op)
    {
        auto result = ws.send(data, sz, WebSock::WsOp(op));
        if (ch)
            chs(ch, bool, result);
    }

    void WebSockApi::broadcast(WebSock* src, const void* data, size_t size)
    {
        strace("WebSockApi::broadcast src %p, data %p, size %lu",
               src, data, size);

        Channel<int> async{-1};
        uint32 wait = 0;

        auto msg = reinterpret_cast<const WsockBcastMsg *>(data);

        for (auto& ws : Ego._webSocks) {
            if (&ws.second.get() != src) {
                if (Ego._blockingBroadcast or Ego._webSocks.size() == 2) {
                    // there is no need to spawn go-routines if there is only
                    // one other sock or in blocking mode
                    auto& wsock = ws.second.get();
                    if (!wsock.broadcastSend(msg->payload, msg->len)) {
                        ltrace(&wsock, "sending web socket message failed");
                    }
                } else {
                    wait++;
                    go(broadcastSend(async, ws.second, msg->payload, msg->len));
                }
            }
        }

        strace("waiting for %u-local to complete %ld", wait, mnow());
        // wait for transactions to complete
        async(wait) | Void;
        strace("web socket broadcast completed %ld", mnow());
    }

    void WebSockApi::broadcastSend(Channel<int>& ch, WebSock& ws, const void* data, size_t len)
    {
        auto result = ws.broadcastSend(data, len);
        if (ch)
            ch << result;
    }

    WebSock::~WebSock() noexcept
    {
        if (Ego._data) {
            free(Ego._data);
            Ego._data = nullptr;
        }
        Ego._endSession = true;
    }

    WebSock::WebSock(net::Socket& sock, WebSockApi& api, size_t size)
        : _sock(sock),
          _api(api)
    {
        if (size) {
            /* allocate data memory */
            Ego._data = calloc(1, size);
        }
    }

    void WebSock::close()
    {
        Ego._endSession = true;
    }

    Status WebSock::handshake(
            const server::Request& req,
            server::Response& resp,
            WebSockApi& api,
            size_t size,
            WebSockCreated onSockCreated)
    {
        strview  key{}, version{};

        key = req.header("Sec-WebSocket-Key");
        if (key.empty()) {
            // we could throw an error here
            return Status::BadRequest;
        }

        version = req.header("Sec-WebSocket-Version");
        if (version.empty() || version != "13") {
            // currently only version 13 supported
            resp.header("Sec-WebSocket-Version", "13");
            return Status::BadRequest;
        }

        Buffer     buf{127};
        buf << key;
        buf << WsServerResponse;
        auto encoded = Base64::encode(String{buf, false});
        resp.header("Upgrade", "WebSocket");
        resp.header("Connection", "Upgrade");
        resp.header("Sec-WebSocket-Accept", std::move(encoded));

        // end the Response by the handler
        resp.end(
        [&api, size, created = std::move(onSockCreated)](server::Request &rq, server::Response &rs) {
            // clear the Request to free resources
            rq.clear();

            // Create a web socket
            WebSock ws(rq.sock(), api, size);
            // notify API that websocket has been created
            if (created)
                created(ws);

            ws.handle();
            return true;
        });

        // return the switching protocols
        return Status::SwitchingProtocols;
    }

    bool WebSock::receiveOpcode(header& h)
    {
        size_t  len{0};
        size_t  nbytes = WS_FRAME_HDR;

        uint16 u16{0};
        if (!Ego._sock.receive(&u16, nbytes, Ego._api._timeout)) {
            itrace("%s - receiving op code failed: %s", sock.id(), errno_s);
            return false;
        }
        h.u16All = u16;

        if (!h.mask) {
            idebug("%s - received frame does not have op code mask", Ego._sock.id());
            return false;
        }

        if (h.rsv1 || h.rsv2 || h.rsv3) {
            idebug("%s - receive has RSV bits set %d:%d:%d",
                   Ego._sock.id(), h.rsv1, h.rsv2, h.rsv3);
            return false;
        }

        switch (h.opcode) {
            case WsOp::Cont:
            case WsOp::Text:
            case WsOp::Binary:
                break;
            case WsOp::Close:
            case WsOp::Ping:
            case WsOp::Pong:
                if (h.len > WS_PAYLOAD_SINGLE || !h.fin) {
                    idebug("%s - frame (%hX) to large or fragmented",
                           Ego._sock.id(), h.u16All);
                    return false;
                }
                break;
            default:
                idebug("%s - unrecognised op code (%hX)",
                       Ego._sock.id(), h.u16All);
                return false;
        }

        uint8 extraBytes{0};
        switch (h.len) {
            case WS_PAYLOAD_EXTEND_1:
                extraBytes = sizeof(uint16);
                break;
            case WS_PAYLOAD_EXTEND_2:
                extraBytes = sizeof(uint64);
                break;
            default:
                break;
        }

        if (extraBytes) {
            uint8 buf[sizeof(uint64)] = {0};
            buf[0] = (uint8) h.len;
            auto read = size_t(extraBytes-1);
            if (!Ego._sock.receive(&buf[1], read, Ego._api._timeout)) {
                itrace("%s - receiving length failed: %s", sock.id(), errno_s);
                return false;
            }
            if (h.len == WS_PAYLOAD_EXTEND_1) {
                len = suil::rd<uint16>(buf);
            }
            else {
                len = suil::rd<uint64>(buf);
            }
        }
        else {
            len = (uint8) h.len;
        }
        h.payloadSize = len;
        // receive the mask
        nbytes = WS_MASK_LEN;
        if (!Ego._sock.receive(h.v_mask, nbytes, Ego._api._timeout)) {
            idebug("%s - receiving mask failed: %s", Ego._sock.id(), errno_s);
            return false;
        }

        return true;
    }

    bool WebSock::receiveFrame(header& h, Buffer& b)
    {
        if (!receiveOpcode(h)) {
            idebug("%s - receiving op code failed", Ego._sock.id());
            return false;
        }

        size_t len = h.payloadSize;
        b.reserve(h.payloadSize+2);
        auto buf = reinterpret_cast<uint8 *>(b.data());
        if (!Ego._sock.receive(buf, len, Ego._api._timeout) || len != h.payloadSize) {
            itrace("%s - receiving web socket frame failed: %s", sock.id(), errno_s);
            return false;
        }

        for (uint i = 0; i < len; i++)
            buf[i] ^= h.v_mask[i%WS_MASK_LEN];
        // advance to end of buffer
        b.seek(len);

        return true;
    }

    void WebSock::handle()
    {
        if (Ego._api.onConnect) {
            Ego._api.onConnect(Ego);
            if (Ego._endSession) {
                // onConnect reject connection
                itrace("%s - websocket Connection rejected", sock.id());
                return;
            }
        }

        Ego._uuid = suil::uuidstr();
        Ego._api._webSocks.emplace(Ego._uuid.peek(), *this);
        Ego._api._totalSocks++;

        idebug("%s - entering Connection loop %lu", Ego._uuid(), Ego._api._totalSocks);

        Buffer b(0);
        while (!Ego._endSession && Ego._sock.isOpen()) {
            header h;
            b.clear();

            // while the adaptor is still open
            if (!receiveFrame(h, b)) {
                // receiving frame failed, abort Connection
                itrace("%s - receive frame failed", ipstr(sock.addr()));
                Ego._endSession = true;
            }
            else {
                switch (h.opcode) {
                    case Pong:
                    case Cont:
                        ierror("%s - web socket op (%02X) not supported",
                               Ego._sock.id(), h.opcode);
                        Ego._endSession = true;

                    case WsOp::Text:
                    case WsOp::Binary:
                            if (Ego._api.onMessage) {
                                // one way of appending null at end of string
                                (char*) b;
                                Ego._api.onMessage(*this, b, (WsOp) h.opcode);
                            }
                        break;
                    case WsOp::Close:
                        Ego._endSession = true;
                        if (Ego._api.onClose) {
                            Ego._api.onClose(*this);
                        }
                        break;
                    case WsOp::Ping:
                        send(b, WsOp::Pong);
                        break;
                    default:
                        itrace("%s - unknown web socket op %02X",
                               Ego._sock.id(), h.opcode);
                        Ego._endSession = true;
                }
            }
        }

        // remove from list of know web sockets
        Ego._api._webSocks.erase(Ego._uuid);
        Ego._api._totalSocks--;

        itrace("%s - done handling web socket %hhu", Ego._uuid(), Ego._api._totalSocks);

        // definitely disconnecting
        if (Ego._api.onDisconnect) {
            Ego._api.onDisconnect();
        }
    }

    bool WebSock::send(const void* data, size_t size, WsOp op)
    {
        uint8 payload_1;
        uint8 hbuf[14] = {0};
        uint8 hlen = WS_FRAME_HDR;

        if (Ego._endSession) {
            itrace("%s - sending while Session is closing is not allow",
                   sock.id());
            return false;
        }

        if (size > WS_PAYLOAD_SINGLE) {
            payload_1 = uint8((size < USHRT_MAX)?
                                   WS_PAYLOAD_EXTEND_1 : WS_PAYLOAD_EXTEND_2);
        }
        else {
            payload_1 = uint8(size);
        }
        auto& h = (header &) hbuf;
        h.u16All  = 0;
        h.opcode  = (op & WS_OPCODE_MASK);
        h.fin     = 1;
        h.len     = (payload_1 & uint8(~(1<<7)));

        if (payload_1 > WS_PAYLOAD_SINGLE) {
            uint8 *p = hbuf + hlen;
            if (payload_1 == WS_PAYLOAD_EXTEND_1) {
                suil::wr<uint16>(p, uint16(size));
                hlen += sizeof(uint16);
            }
            else {
                suil::wr<uint64>(p, uint64(size));
                hlen += sizeof(uint64);
            }
        }

        // send header
        if (Ego._sock.send(hbuf, hlen, Ego._api._timeout) != hlen) {
            itrace("%s - sending header of length %hhu failed: %s",
                   Ego._sock.id(), hlen, errno_s);
            return false;
        }

        // send the reset of the message
        if (Ego._sock.send(data, size, Ego._api._timeout) != size) {
            itrace("%s - sending data of length %lu failed: %s",
                   Ego._sock.id(), size, errno_s);
            return false;
        }

        return Ego._sock.flush(Ego._api._timeout);
    }

    bool WebSock::broadcastSend(const void* data, size_t len)
    {
        if (!Ego._sock.isOpen()) {
            iwarn("attempting to send to a closed websocket");
            return false;
        }
        size_t totalSent = 0;
        do {
            auto sent = Ego._sock.send(data, len, Ego._api._timeout);
            if (!sent) {
                itrace("sending websocket data failed: %s", errno_s);
                return false;
            }

            totalSent += sent;
        } while (totalSent < len);

        return Ego._sock.flush(Ego._api._timeout);
    }

    void WebSock::broadcast(const void* data, size_t sz, WsOp op)
    {
        itrace("WebSock::broadcast data %p, sz %lu, op 0x%02X", data, sz, op);

        // only broadcast when there are other web socket clients
        if (Ego._api._totalSocks > 0) {
            itrace("broadcasting %lu web sockets", Ego._api._totalSocks);
            auto copy = (uint8 *) malloc(
                    sizeof(WsockBcastMsg) + sz + 16);
            auto msg = reinterpret_cast<WsockBcastMsg *>(copy);

            uint8 payload_1;
            uint8 hlen = WS_FRAME_HDR;

            if (sz > WS_PAYLOAD_SINGLE) {
                payload_1 = uint8((sz < USHRT_MAX)?
                                       WS_PAYLOAD_EXTEND_1 : WS_PAYLOAD_EXTEND_2);
            }
            else {
                payload_1 = uint8(sz);
            }
            auto h = (header &) msg->payload;
            h.u16All  = 0;
            h.opcode  = (op & WS_OPCODE_MASK);
            h.fin     = 1;
            h.len     = (payload_1 & uint8(~(1<<7)));

            if (payload_1 > WS_PAYLOAD_SINGLE) {
                uint8_t *p = ((uint8_t *) msg->payload) + hlen;
                if (payload_1 == WS_PAYLOAD_EXTEND_1) {
                    suil::wr<uint16>(p, uint16(sz));
                    hlen += sizeof(uint16);
                }
                else {
                    suil::wr<uint64>(p, uint64(sz));
                    hlen += sizeof(uint64);
                }
            }
            uint8 *tmp = &msg->payload[hlen];
            memcpy(tmp, data, sz);

            msg->len = sz + hlen;
            msg->api_id = Ego._api._id;
            // the copied buffer now belongs to the go-routine
            // being scheduled
            size_t len = sizeof(WsockBcastMsg)+msg->len;
            go(broadcast(*this, Ego._api, copy, len));
        }
    }

    void WebSock::broadcast(WebSock& ws, WebSockApi& api, void* data, size_t size)
    {
        strace("WebSock::broadcast data %p size %lu", data, size);
        // use the api to broadcast to all connected web sockets
        api.broadcast(&ws, data, size);
        // free the allocated memory
        strace("done broadcasting message %p", data);
        free(data);
    }
}
