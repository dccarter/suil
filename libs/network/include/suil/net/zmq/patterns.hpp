//
// Created by Mpho Mbotho on 2020-12-11.
//

#ifndef LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_PATTERNS_HPP
#define LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_PATTERNS_HPP

#include <suil/net/zmq/operators.hpp>
#include <suil/net/zmq/socket.hpp>

namespace suil::net::zmq {

    using ReplySocket   = OperatedSocket<Socket::Rep,
                            BindOperator, SendOperator, ReceiveOperator, MonitorOperator>;
    using RequestSocket = OperatedSocket<Socket::Type::Req,
                            ConnectOperator, SendOperator, ReceiveOperator, MonitorOperator>;

    using DealerSocket = OperatedSocket<Socket::Type::Req,
                            BindOperator, ConnectOperator, SendOperator, ReceiveOperator, MonitorOperator>;

    using RouterSocket = OperatedSocket<Socket::Type::Req,
                            BindOperator, ConnectOperator, SendOperator, ReceiveOperator, MonitorOperator>;


    using PublishSocket = OperatedSocket<Socket::Pub,
                            BindOperator, SendOperator, MonitorOperator>;

    using PairSocket    = OperatedSocket<Socket::Pair,
                            BindOperator, SendOperator, MonitorOperator>;


    class SubscribeSocket : public OperatedSocket<Socket::Sub, ConnectOperator, ReceiveOperator, MonitorOperator> {
    public:
        using OperatedSocket<Socket::Sub, ConnectOperator, ReceiveOperator, MonitorOperator>::OperatedSocket;

        ~SubscribeSocket() override = default;

        MOVE_CTOR(SubscribeSocket) = default;
        MOVE_ASSIGN(SubscribeSocket) = default;

        template <typename... Topics>
        bool subscribe(const String& topic, const Topics&... topics) {
            if (!doSubscribe(topic)) {
                return false;
            }
            if constexpr (sizeof...(topics) > 0) {
                return subscribe(topics...);
            }
        }


        template <typename ...Topics>
        void unsubscribe(const String& topic, const Topics&... topics) {
            doUnsubscribe(topic);
            if constexpr (sizeof...(topics) > 0) {
                return unsubscribe(topics...);
            }
        }

    private:
        DISABLE_COPY(SubscribeSocket);

        bool doSubscribe(const String& topic);
        void doUnsubscribe(const String& topic);
    };

}
#endif //LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_PATTERNS_HPP
