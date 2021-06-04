//
// Created by Mpho Mbotho on 2020-12-20.
//

#ifndef SUIL_HTTP_SERVER_WEBSOCK_HPP
#define SUIL_HTTP_SERVER_WEBSOCK_HPP

#include <suil/http/server/request.hpp>
#include <suil/http/server/response.hpp>

#include <suil/base/channel.hpp>
#include <suil/base/signal.hpp>

#include <libmill/libmill.hpp>

namespace suil::http {

    class WebSock;

    class WebSockApi {
    public:
        WebSockApi(int64 timeout, bool blockingBroadcast = true);

        using ConnectHandler    = Signal<void(WebSock&)>;
        using CloseHandler      = Signal<void(WebSock&)>;
        using DisconnectHandler = Signal<void()>;
        using MessageHandler    = Signal<void(WebSock&, const Buffer&, uint8)>;

        DisconnectHandler onDisconnect;
        ConnectHandler onConnect;
        CloseHandler   onClose;
        MessageHandler onMessage;

        std::shared_ptr<WebSock> find(const String& uuid);

    private:
        friend class WebSock;
        void broadcast(WebSock* src, const void *data, size_t size);
        static coroutine void   send(chan ch, WebSock& ws, const void *data, size_t sz, uint8 op);
        using WebSockMap = UnorderedMap<std::shared_ptr<WebSock>>;
        using WebSockSnapshot = std::vector<std::weak_ptr<WebSock>>;
        WebSockSnapshot snapshot();
        WebSockMap   _webSocks{};
        size_t  _totalSocks{0};
        uint8 _id{0};
        int64  _timeout{-1};
        mill::Mutex _mutex{};
        bool _blockingBroadcast{true};
    };

    struct WsockBcastMsg {
        uint8_t         api_id;
        size_t          len;
        uint8_t         payload[0];
    } __attribute((packed));

#define IPC_WSOCK_BCAST ipc_msg(1)
#define IPC_WSOCK_CONN  ipc_msg(2)

    /**
     * The web socket Connection message. This message is sent to other
     * worker whenever a worker receives a new web socket Connection and
     * whenever a socket disconnects
     */
    struct WsockConnMsg {
        uint8_t         api_id;
        uint8_t         conn;
    } __attribute((packed));

    using WebSockCreated = std::function<void(WebSock&)>;

    define_log_tag(WEB_SOCKET);
    class WebSock : LOGGER(WEB_SOCKET), public std::enable_shared_from_this<WebSock> {
    public:
        typedef enum  : uint8_t  {
            Cont    = 0x00,
            Text    = 0x01,
            Binary  = 0x02,
            Close   = 0x08,
            Ping    = 0x09,
            Pong    = 0x0A
        } WsOp;

        bool send(const void *, size_t, WsOp);

        bool send(const void *data, size_t size) {
            return send(data, size, WsOp::Binary);
        }
        bool send(const String& zc, WsOp op = WsOp::Text) {
            return send(zc.data(), zc.size(), op);
        }

        inline bool send(const Data& b, WsOp op = WsOp::Binary) {
            return send(b.data(), b.size(), op);
        }

        void broadcast(const void *data, size_t sz, WsOp op);

        inline void broadcast(const String& zc, WsOp op = WsOp::Text) {
            broadcast(zc.data(), zc.size(), op);
        }

        inline void broadcast(const Data& b, WsOp op = WsOp::Binary) {
            broadcast(b.data(), b.size(), op);
        }

        void close();

        ~WebSock();

        template <typename Data>
        inline Data* data() {
            return reinterpret_cast<Data*>(Ego._data);
        }

        static Status handshake(
                const server::Request&,
                server::Response&,
                WebSockApi&,
                size_t,
                WebSockCreated onSockCreated);

    protected:
        WebSock(net::Socket& sock, WebSockApi& api, size_t size = 0);

        template <typename ...Mws>
        friend struct Connection;
        struct  header {
            header()
                : u16All(0),
                  payloadSize(0)
            {
                memset(v_mask, 0, sizeof(v_mask));
            }

            union {
                struct {
                    struct {
                        uint8_t opcode   : 4;
                        uint8_t rsv3     : 1;
                        uint8_t rsv2     : 1;
                        uint8_t rsv1     : 1;
                        uint8_t fin      : 1;
                    } __attribute__((packed));
                    struct {
                        uint8_t len      : 7;
                        uint8_t mask     : 1;
                    } __attribute__((packed));
                };
                uint16_t u16All;
            };
            size_t          payloadSize;
            uint8_t         v_mask[4];
        } __attribute__((packed));

        virtual bool receiveOpcode(header& h);
        virtual bool receiveFrame(header& h, Buffer& b);
        net::Socket&      _sock;
        WebSockApi&       _api;
        bool              _endSession{false};
        void              *_data{nullptr};
        String             _uuid{};
        mill::Mutex _mutex{};
    private:
        friend struct WebSockApi;
        void handle();
        bool broadcastSend(const void *data, size_t len);
        static coroutine void broadcast(
                std::shared_ptr<WebSock> ws, WebSockApi& api, void *data, size_t size);
    };

    namespace ws {
        template <typename T = Void_t>
        inline void handshake(
                const server::Request& req,
                server::Response& resp,
                WebSockApi& api,
                WebSockCreated onSockCreated = nullptr)
        {
            auto s = WebSock::handshake(
                                req,
                                resp,
                                api,
                                sizeof(T),
                                std::move(onSockCreated));
            resp.end(s);
        }

        template <typename... Ops>
        WebSockApi api(Ops&&... ops) {
            auto opts = iod::D(std::forward<Ops>(ops)...);

            WebSockApi api{
                    opts.get(sym(timeout), int64{-1}),
                    opts.get(sym(blockingBroadcast), false)
            };

            if (opts.has(sym(onConnect))) {
                api.onConnect += std::move(opts.get(sym(onConnect), nullptr));
            }

            if (opts.has(sym(onClose))) {
                api.onClose += std::move(opts.get(sym(onClose), nullptr));
            }

            if (opts.has(sym(onMessage))) {
                api.onMessage += std::move(opts.get(sym(onMessage), nullptr));
            }

            if (opts.has(sym(onDisconnect))) {
                api.onDisconnect += std::move(opts.get(sym(onDisconnect), nullptr));
            }

            return std::move(api);
        }
    }
}
#endif //SUIL_HTTP_SERVER_WEBSOCK_HPP
