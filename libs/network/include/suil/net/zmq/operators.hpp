//
// Created by Mpho Mbotho on 2020-12-11.
//

#ifndef LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_OPERATORS_HPP
#define LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_OPERATORS_HPP

#include <suil/net/zmq/message.hpp>

#include <suil/net/zmq/common.hpp>

namespace suil::net::zmq {

    class Socket;

    class SendOperator {
    public:
        virtual Socket& sock() = 0;

        bool sendFlags(const Message& msg, int flags, const Deadline& dd = Deadline::Inf);

        inline bool send(const Message& msg, const Deadline& dd = Deadline::Inf) {
            return Ego.sendFlags(msg, 0, dd);
        }

        bool sendFlags(const void* buf, size_t len, int flags, const Deadline& dd = Deadline::Inf);

        inline bool send(const void* buf, size_t len, const Deadline& dd = Deadline::Inf) {
            return Ego.sendFlags(buf, len, 0 /* normal flags */, dd);
        }

        template <DataBuf D>
        inline bool sendFlags(const D& buf, int flags, const Deadline& dd = Deadline::Inf) {
            return Ego.sendFlags(buf.data(), buf.size(), flags, dd);
        }

        template <DataBuf D>
        inline bool send(const D& buf, const Deadline& dd = Deadline::Inf) {
            return Ego.sendFlags(buf, 0 /* normal flags */, dd);
        }
    };

    class ReceiveOperator {
    public:
        virtual Socket& sock() = 0;

        bool receiveFlags(Message& msg, int flags, const Deadline& dd = Deadline::Inf);
        bool receive(Message& msg, const Deadline& dd = Deadline::Inf) {
            return receiveFlags(msg, 0 /* normal flags */, dd);
        }

        bool receiveFlags(void *buf, size_t& len, int flags, const Deadline& dd = Deadline::Inf);
        inline bool receive(void *buf, size_t& len, const Deadline& dd = Deadline::Inf) {
            return Ego.receiveFlags(buf, len, 0 /* normal flags */, dd);
        }
    };

    class ConnectOperator {
    public:
        virtual Socket& sock() = 0;

        template <typename... Endpoints>
        bool connect(const String& endpoint, const Endpoints&... endpoints) {
            if (!doConnect(endpoint)) {
                return false;
            }
            if constexpr (sizeof...(endpoints) > 0) {
                return connect(endpoints...);
            }

            return true;
        }

        template <typename... Endpoints>
        void disconnect(const String& endpoint, const Endpoints&... endpoints) {
            doDisconnect(endpoint);
            if constexpr (sizeof...(endpoints) > 0) {
                disconnect(endpoints...);
            }
        }

    private:
        bool doConnect(const String& endpoint);
        void doDisconnect(const String& endpoint);
    };

    class BindOperator {
    public:
        virtual Socket& sock() = 0;

        bool bind(const String& endpoint);

#if (ZMQ_VERSION_MAJOR > 3) || ((ZMQ_VERSION_MAJOR == 3) && (ZMQ_VERSION_MINOR >= 2))
        void unbind(const String& endpoint);
#endif
    };


    class MonitorOperator {
    public:
        virtual Socket& sock() = 0;
#if (ZMQ_VERSION_MAJOR >= 4)
        bool monitor(const String& me, int events);
        void unMonitor();
#endif
    };

}

#endif //LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_OPERATORS_HPP
