//
// Created by Mpho Mbotho on 2020-12-08.
//

#ifndef LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_SOCKET_HPP
#define LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_SOCKET_HPP

#include <zmq.h>

#include <suil/base/string.hpp>
#include <suil/base/logging.hpp>

#include <suil/net/zmq/common.hpp>

namespace suil::net::zmq {

    class Message;
    class Context;

    class Socket : public LOGGER(ZMQ_SOCK) {
    public:
        static constexpr int MaxSocketOptionBufferSize{256};
        typedef enum {
            Inval   = -1,
            Pair    = ZMQ_PAIR,
            Pub     = ZMQ_PUB,
            Sub     = ZMQ_SUB,
            Pull    = ZMQ_SUB,
            Req     = ZMQ_REQ,
            Rep     = ZMQ_REP,
            XPub    = ZMQ_XPUB,
            XSub    = ZMQ_XSUB,
            XReq    = ZMQ_XREP,
            Router  = ZMQ_ROUTER,
            Dealer  = ZMQ_DEALER
        } Type;

        Socket(Context& ctx, Type type);

        virtual ~Socket();

        MOVE_CTOR(Socket) noexcept;
        MOVE_ASSIGN(Socket) noexcept;

        void close();

        operator bool() const;

        void* raw() {
            return _raw;
        }

        int fd();

        const String& id() const {
            return _id;
        }

        bool set(int opt, const void *buf, size_t len);
        std::pair<bool, size_t> get(int opt, void *buf, size_t len) const;

    private:
        DISABLE_COPY(Socket);
        void  createIdentity();
        void* _raw{nullptr};
        Type  _type{Inval};
        int   _fd{-1};
        String _id{};
    };

    template <Socket::Type type, typename... Ops>
    class OperatedSocket : public Socket, public Ops... {
    public:
        OperatedSocket(Context& ctx) : Socket(ctx, type) {}

        virtual ~OperatedSocket() = default;

        Socket& sock() override { return Ego; }

        MOVE_CTOR(OperatedSocket) = default;
        MOVE_ASSIGN(OperatedSocket) = default;

    private:
        DISABLE_COPY(OperatedSocket);
    };

#define _SETTER(Opt, Tp) static inline bool set##Opt(Socket& sock, const Tp& val) { return Option::set(sock, Opt , val ) ; }
#define _GETTER(Opt, Tp) static inline bool get##Opt(const Socket& sock, Tp& val) { return Option::get(sock, Opt , val ) ; }
#define _GETSET(Opt, Tp) \
    _SETTER(Opt, Tp)     \
    _GETTER(Opt, Tp)


    ///< See from https://github.com/zeromq/zmqpp/blob/develop/src/zmqpp/socket_options.hpp
    ///<     http://api.zeromq.org/4-1:zmq-setsockopt
    ///<     http://api.zeromq.org/4-1:zmq-getsockopt
    class Option {
    public:
        static constexpr int Affinity                  = ZMQ_AFFINITY;          /*!< I/O thread affinity */
        _GETSET(Affinity, uint64)

        static constexpr int Identity                  = ZMQ_IDENTITY;          /*!< Socket identity */
        _GETSET(Identity, suil::String)

        static constexpr int Subscribe                 = ZMQ_SUBSCRIBE;         /*!< Add topic subscription - set only */
        _GETSET(Subscribe, suil::String);

        static constexpr int Unsubscribe               = ZMQ_UNSUBSCRIBE;       /*!< Remove topic subscription - set only */
        _GETSET(Unsubscribe, suil::String);

        static constexpr int Rate                      = ZMQ_RATE;              /*!< Multicast data rate */
        _GETSET(Rate, unsigned int);

        static constexpr int SendBufferSize            = ZMQ_SNDBUF;            /*!< Kernel transmission buffer size */
        _GETSET(SendBufferSize, unsigned int);

        static constexpr int ReceiveBufferSize         = ZMQ_RCVBUF;            /*!< Kernel receive buffer size */
        _GETSET(ReceiveBufferSize, unsigned int);

        static constexpr int ReceiveMore               = ZMQ_RCVMORE;           /*!< Can receive more parts - get only */
#if (ZMQ_VERSION_MAJOR == 2)
        _GETTER(ReceiveMore, int64);
#else
        _GETTER(ReceiveMore, int);
#endif
        static constexpr int FileDescriptor            = ZMQ_FD;                /*!< Socket file descriptor - get only */
        _GETTER(FileDescriptor, int);

        static constexpr int Events                    = ZMQ_EVENTS;            /*!< Socket event flags - get only */
        _GETTER(Events, int);

        static constexpr int Type                      = ZMQ_TYPE;              /*!< Socket type - get only */
        _GETTER(Type, int);

        static constexpr int Linger                    = ZMQ_LINGER;            /*!< Socket linger timeout */
        _GETSET(Linger, int);

        static constexpr int Backlog                   = ZMQ_BACKLOG;           /*!< Maximum length of outstanding connections - get only */
        _GETSET(Backlog, unsigned int);

        static constexpr int ReconnectInterval         = ZMQ_RECONNECT_IVL;     /*!< Reconnection interval */
        _GETSET(ReconnectInterval, int);

        static constexpr int ReconnectInterval_max     = ZMQ_RECONNECT_IVL_MAX; /*!< Maximum reconnection interval */
        _GETSET(ReconnectInterval_max, unsigned int);

        static constexpr int ReceiveTimeout            = ZMQ_RCVTIMEO;          /*!< Maximum inbound block timeout */
        _GETSET(ReceiveTimeout, int);

        static constexpr int SendTimeout               = ZMQ_SNDTIMEO;          /*!< Maximum outbound block timeout */
        _GETSET(SendTimeout, int);

#if (ZMQ_VERSION_MAJOR == 2)
        // Note that this is inverse of the zmq names for version 2.x
        static constexpr int RecoveryIntervalSeconds   = ZMQ_RECOVERY_IVL;      /*!< Multicast recovery interval in seconds */
        _GETSET(RecoveryIntervalSeconds, int64);

        static constexpr int RecoveryInterval          = ZMQ_RECOVERY_IVL_MSEC; /*!< Multicast recovery interval in milliseconds */
        _GETSET(RecoveryInterval, int);

        static constexpr int HighWaterMark             = ZMQ_HWM;               /*!< High-water mark for all messages */
        _GETSET(HighWaterMark, uint64);

        static constexpr int SwapSize                  = ZMQ_SWAP;              /*!< Maximum socket swap size in bytes */
        _GETSET(SwapSize, int64);

        static constexpr int MulticastLoopback         = ZMQ_MCAST_LOOP;        /*!< Allow multicast packet loopback */
        _GETSET(MulticastLoopback, bool);

#else // version > 2
        static constexpr int RecoveryInterval          = ZMQ_RECOVERY_IVL;      /*!< Multicast recovery interval in milliseconds */
        _GETSET(RecoveryInterval, unsigned int);

        static constexpr int MaxMessageSize            = ZMQ_MAXMSGSIZE;        /*!< Maximum inbound message size */
        _GETSET(MaxMessageSize, int64);

        static constexpr int SendHighWaterMark         = ZMQ_SNDHWM;            /*!< High-water mark for outbound messages */
        _GETSET(MaxMessageSize, unsigned int);

        static constexpr int ReceiveHighWaterMark      = ZMQ_RCVHWM;            /*!< High-water mark for inbound messages */
        _GETSET(SendHighWaterMark, unsigned int);

        static constexpr int MulticastHops             = ZMQ_MULTICAST_HOPS;    /*!< Maximum number of multicast hops */
        _GETSET(MulticastHops, unsigned int);

#if (ZMQ_VERSION_MAJOR > 3) || ((ZMQ_VERSION_MAJOR == 3) && (ZMQ_VERSION_MINOR >= 1))
        static constexpr int Ipv4Only                  = ZMQ_IPV4ONLY;
        _GETSET(Ipv4Only, bool);

#endif
#if (ZMQ_VERSION_MAJOR > 3) || ((ZMQ_VERSION_MAJOR == 3) && (ZMQ_VERSION_MINOR >= 2))
#if ((ZMQ_VERSION_MAJOR == 3) && (ZMQ_VERSION_MINOR == 2))
        statc constexpr int DelayAttachOnConnect      = ZMQ_DELAY_ATTACH_ON_CONNECT; /*!< Delay buffer attachment until connect complete */
        _GETSET(DelayAttachOnConnect, bool);
#else
        //  ZMQ_DELAY_ATTACH_ON_CONNECT is renamed in ZeroMQ starting 3.3.x
        static constexpr int Immediate                 = ZMQ_IMMEDIATE;               /*!< Block message sending until connect complete */
        _GETSET(Immediate, bool);
#endif
        static constexpr int LastEndpoint              = ZMQ_LAST_ENDPOINT;           /*!< Last bound endpoint - get only */
        _GETTER(LastEndpoint, suil::String)

        static constexpr int RouterMandatory           = ZMQ_ROUTER_MANDATORY;        /*!< Require routable messages - set only */
        _GETSET(RouterMandatory, bool);

        static constexpr int XpubVerbose               = ZMQ_XPUB_VERBOSE;            /*!< Pass on existing subscriptions - set only */
        _GETSET(XpubVerbose, bool);

        static constexpr int TcpKeepalive              = ZMQ_TCP_KEEPALIVE;           /*!< Enable TCP keepalives */
        _GETSET(TcpKeepalive, int);

        static constexpr int TcpKeepaliveIdle          = ZMQ_TCP_KEEPALIVE_IDLE;      /*!< TCP keepalive idle count (generally retry count) */
        _GETSET(TcpKeepaliveIdle, int);

        static constexpr int TcpKeepaliveCount         = ZMQ_TCP_KEEPALIVE_CNT;       /*!< TCP keepalive retry count */
        _GETSET(TcpKeepaliveCount, int);

        static constexpr int TcpKeepaliveInterval      = ZMQ_TCP_KEEPALIVE_INTVL;     /*!< TCP keepalive interval */
        _GETSET(TcpKeepaliveInterval, int);

        static constexpr int TcpAcceptFilter           = ZMQ_TCP_ACCEPT_FILTER;       /*!< Filter inbound connections - set only */
        _GETSET(TcpAcceptFilter, suil::String);
#endif

#if (ZMQ_VERSION_MAJOR >= 4)
        static constexpr int Ipv6                      = ZMQ_IPV6; /*!< IPv6 socket support status */
        _GETSET(Ipv6, bool);

        static constexpr int Mechanism                 = ZMQ_MECHANISM; /*!< Socket security mechanism - get only */
        _GETSET(Mechanism, int);

        static constexpr int PlainPassword             = ZMQ_PLAIN_PASSWORD; /*!< PLAIN password */
        _GETSET(PlainPassword, suil::String);

        static constexpr int PlainServer               = ZMQ_PLAIN_SERVER; /*!< PLAIN server role */
        _GETSET(PlainServer, bool);

        static constexpr int PlainUsername             = ZMQ_PLAIN_USERNAME; /*!< PLAIN username */
        _GETSET(PlainUsername, suil::String);

        static constexpr int ZapDomain                 = ZMQ_ZAP_DOMAIN; /*!< RFC 27 authentication domain */
        _GETSET(ZapDomain, suil::String);

        static constexpr int Conflate                  = ZMQ_CONFLATE; /*!< Keep only last message - set only */
        static constexpr int CurvePublicKey            = ZMQ_CURVE_PUBLICKEY; /*!< CURVE public key */
        _GETSET(CurvePublicKey, suil::String);

        static constexpr int CurveSecretKey            = ZMQ_CURVE_SECRETKEY; /*!< CURVE secret key */
        _GETSET(CurveSecretKey, suil::String);

        static constexpr int CurveServerKey            = ZMQ_CURVE_SERVERKEY; /*!< CURVE server key */
        _GETSET(CurveServerKey, suil::String);

        static constexpr int CurveServer               = ZMQ_CURVE_SERVER; /*!< CURVE server role - set only */
        _GETSET(CurveServer, bool);

        static constexpr int ProbeRouter               = ZMQ_PROBE_ROUTER; /*!< Bootstrap connections to ROUTER sockets - set only */
        _GETSET(ProbeRouter, bool);

        static constexpr int RequestCorrelate          = ZMQ_REQ_CORRELATE; /*!< Match replies with requests - set only */
        _GETSET(RequestCorrelate, bool);

        static constexpr int RequestRelaxed            = ZMQ_REQ_RELAXED; /*!< Relax strict alternation between request and reply - set only */
        _GETSET(RequestRelaxed, bool);

        static constexpr int RouterRaw                 = ZMQ_ROUTER_RAW; /*!< Switch ROUTER socket to raw mode - set only */
        _GETSET(RouterRaw, bool);
#endif

#if (ZMQ_VERSION_MAJOR > 4) || ((ZMQ_VERSION_MAJOR == 4) && (ZMQ_VERSION_MINOR >= 1))
        static constexpr int HandshakeInterval        = ZMQ_HANDSHAKE_IVL; /*!< Maximum handshake interval */
        _GETSET(HandshakeInterval, int);

        static constexpr int TypeOfService            = ZMQ_TOS; /*!< Type-of-Service socket override status */
        _GETSET(TypeOfService, unsigned int)

        static constexpr int ConnectRid               = ZMQ_CONNECT_RID; /*!< Assign the next outbound connection id - set only */
        _SETTER(ConnectRid, suil::String);

        static constexpr int IpcFilterGid             = ZMQ_IPC_FILTER_GID; /*!< Group ID filters to allow new IPC connections - set only */
        _SETTER(IpcFilterGid, gid_t)

        static constexpr int IpcFilterPid             = ZMQ_IPC_FILTER_PID; /*!< Process ID filters to allow new IPC connections - set only */
        _SETTER(IpcFilterPid, pid_t)

        static constexpr int IpcFilterUid             = ZMQ_IPC_FILTER_UID; /*!< User ID filters to allow new IPC connections - set only */
        _SETTER(IpcFilterUid, uid_t)

        static constexpr int RouterHandover           = ZMQ_ROUTER_HANDOVER; /*!< Handle duplicate client identities on ROUTER sockets - set only */
        _SETTER(RouterHandover, bool);

#endif
#if (ZMQ_VERSION_MAJOR > 4) || ((ZMQ_VERSION_MAJOR == 4) && (ZMQ_VERSION_MINOR >= 2))
        static constexpr int ConnectTimeout           = ZMQ_CONNECT_TIMEOUT; /*< Connect system call timeout */
        _GETSET(ConnectTimeout, int);

        static constexpr int GssapiPlaintext          = ZMQ_GSSAPI_PLAINTEXT; /*< GSSAPI plaintext (disabled) state */
        _GETSET(GssapiPlaintext, bool);

        static constexpr int GssapiPrincipal          = ZMQ_GSSAPI_PRINCIPAL; /*< GSSAPI principal name */
        _GETSET(GssapiPrincipal, suil::String);

        static constexpr int GssapiServer             = ZMQ_GSSAPI_SERVER; /*< GSSAPI server state */
        _GETSET(GssapiServer, bool);

        static constexpr int GssapiServicePrincipal   = ZMQ_GSSAPI_SERVICE_PRINCIPAL; /*< GSSAPI connected server principal name */
        _GETSET(GssapiServicePrincipal, suil::String);

        static constexpr int HeartbeatInterval        = ZMQ_HEARTBEAT_IVL; /*< Heartbeat interval for ZMPT - set only */
        _GETSET(HeartbeatInterval, int);

        static constexpr int HeartbeatTimeout         = ZMQ_HEARTBEAT_TIMEOUT; /*< ZMPT heartbeat timeout - set only */
        _GETSET(HeartbeatTimeout, int);

        static constexpr int HeartbeatTtl             = ZMQ_HEARTBEAT_TTL; /*< ZMPT heartbeat interval - set only */
        _GETSET(HeartbeatTtl, int);

        static constexpr int InvertMatching           = ZMQ_INVERT_MATCHING; /*< ZMPT invert state for PUB/SUB message filters */
        _GETSET(InvertMatching, bool);

        static constexpr int MulticastMaxTpdu         = ZMQ_MULTICAST_MAXTPDU; /*< Max size for multicast messages */
        _GETSET(MulticastMaxTpdu, int);

        static constexpr int SocksProxy               = ZMQ_SOCKS_PROXY; /*< SOCKS5 proxy address for routing tcp connections */
        _GETSET(SocksProxy, suil::String);

        static constexpr int StreamNotify             = ZMQ_STREAM_NOTIFY; /*< Event state on connect/disconnection of peers */
        _GETSET(StreamNotify, bool);

        static constexpr int TpcMaxRetransmit         = ZMQ_TCP_MAXRT; /*< Maximum retransmit timeout */
        _GETSET(TpcMaxRetransmit, int);

        static constexpr int UseFd                    = ZMQ_USE_FD; /*!< Use a pre-allocated file descriptor instead of allocating a new one */
        _GETSET(UseFd, int);

        static constexpr int VmciBufferSize           = ZMQ_VMCI_BUFFER_SIZE; /*< VMCI buffer size */
        _GETSET(VmciBufferSize, uint64);

        static constexpr int VmciBufferMin            = ZMQ_VMCI_BUFFER_MIN_SIZE; /*< VMCI minimum buffer size */
        _GETSET(VmciBufferMin, uint64);

        static constexpr int VmciBufferMax            = ZMQ_VMCI_BUFFER_MAX_SIZE; /*< VMCI maximum buffer size */
        _GETSET(VmciBufferMax, uint64);

        static constexpr int VmciConnectTimeout       = ZMQ_VMCI_CONNECT_TIMEOUT; /*< VMCI connection attempt timeout */
        _GETSET(VmciConnectTimeout, int);

        static constexpr int XpubManual               = ZMQ_XPUB_MANUAL;
        _GETSET(XpubManual, bool);

        static constexpr int XpubNodrop               = ZMQ_XPUB_NODROP;
        _GETSET(XpubNodrop, bool);

        static constexpr int XpubVerboser             = ZMQ_XPUB_VERBOSER; /*!< Pass on existing (un)subscriptions - set only */
        _GETSET(XpubVerboser, bool);

        static constexpr int XpubWelcomeMessage       = ZMQ_XPUB_WELCOME_MSG;
        _GETSET(XpubWelcomeMessage, suil::String);
#endif
#endif // version > 2

#ifdef ZMQ_EXPERIMENTAL_LABELS
        static constexpr int ReceiveLabel             = ZMQ_RCVLABEL;          /*!< Received label part - get only */
#endif

    private:
        static bool set(Socket& sock, int opt, bool v) {
#if (ZMQ_VERSION_MAJOR == 2)
            int64 value = v? 1 : 0;
	        auto len = sizeof(int64);
#else
            int value = v? 1: 0;
            auto len = sizeof(int);
#endif
            return sock.set(opt, &value, len);
        }

        static inline bool set(Socket& sock, int opt, const String& v) {
            return sock.set(opt, v.data(), v.size());
        }

        static bool get(const Socket& sock, int opt, String& v) {
            char buf[Socket::MaxSocketOptionBufferSize] = {0};
            auto [r, s] = sock.get(opt, buf, Socket::MaxSocketOptionBufferSize);
            if (r) {
                v = String{buf, s, false}.dup();
                return true;
            }
            return false;
        }

        template <std::integral T>
        static inline bool set(Socket& sock, int opt, const T& val) {
            if constexpr (std::is_same_v<T, unsigned int>) {
                // there is no unsigned int
                int tmp = static_cast<int>(val);
                return sock.set(opt, &tmp, sizeof(tmp));
            }
            else {
                return sock.set(opt, &val, sizeof(val));
            }
        }

        static bool get(const Socket& sock, int opt, bool& v) {
#if (ZMQ_VERSION_MAJOR == 2)
            int64 value{0};
	        auto len = sizeof(int64);
#else
            int value{0};
            auto len = sizeof(int);
#endif
            auto [r, _] = sock.get(opt, &value, len);
            if (r) {
                v = value != 0;
                return true;
            }
            return false;
        }

        template <std::integral T>
        static inline bool get(const Socket& sock, int opt, T& val) {
            auto [r, _]  = sock.get(opt, &val, sizeof(val));
            return r;
        }
    };
}
#endif //LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_SOCKET_HPP
