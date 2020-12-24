//
// Created by Mpho Mbotho on 2020-12-12.
//

#ifndef LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_MONITOR_HPP
#define LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_MONITOR_HPP

#include <suil/net/zmq/patterns.hpp>

#include <suil/base/channel.hpp>
#include <suil/base/signal.hpp>

namespace suil::net::zmq {

    class SocketMonitor : LOGGER(ZMQ_SOCK) {
    public:
        SocketMonitor(Context& ctx);
        ~SocketMonitor();

        MOVE_CTOR(SocketMonitor) = default;
        MOVE_ASSIGN(SocketMonitor) = default;

        bool start(MonitorOperator& sock, const String& me, int events);

        void stop();

        Signal<void(int16, int32, const String&)> onEvent;

    private:
        DISABLE_COPY(SocketMonitor);
        struct Event {
            int16 ev;
            int32 value;
        } __attribute__((packed));

        static void waitForEvents(SocketMonitor& S);

        // Only need a pair socket that can connect and receive
        OperatedSocket<Socket::Pair, ConnectOperator, ReceiveOperator> _sock;
        bool _connected{false};
        bool _waiting{false};
        Conditional _waiterCond;
    };
}

#endif //LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_MONITOR_HPP
