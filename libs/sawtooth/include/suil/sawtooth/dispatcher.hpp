//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/net/zmq/patterns.hpp>
#include <suil/sawtooth/stream.hpp>

namespace suil::saw {

    namespace Client { struct ValidatorContext; }

    define_log_tag(SAWSDK_DISPATCHER);

    DECLARE_EXCEPTION(DispatcherError);

    struct Dispatcher : LOGGER(SAWSDK_DISPATCHER) {
    sptr(Dispatcher);

        Dispatcher(Dispatcher&&) = delete;
        Dispatcher(const Dispatcher&) = delete;
        Dispatcher&operator=(Dispatcher&&) = delete;
        Dispatcher&operator=(const Dispatcher&) = delete;

        static constexpr Message::Type SERVER_CONNECT_EVENT = static_cast<Message::Type>(0xFFFE);
        static constexpr Message::Type SERVER_DISCONNECT_EVENT = static_cast<Message::Type>(0XFFFF);

        void connect(const suil::String& connString);
        void bind();

        Stream createStream();

        void disconnect();
        void exit();
    protected:
        friend struct TpContext;
        friend struct Client::ValidatorContext;
        friend class TransactionProcessor;
        Dispatcher(net::zmq::Context& ctx);

    private:
        friend class TransactionProcessor;
        static const suil::String DISPATCH_THREAD_ENDPOINT;
        static const suil::String SERVER_MONITOR_ENDPOINT;
        static const suil::String EXIT_MESSAGE;

        static void receiveMessages(Dispatcher& self);
        static void sendMessages(Dispatcher& self);
        static void exitMonitor(Dispatcher& Self);

        net::zmq::Context& mContext;
        net::zmq::DealerSocket mServerSock;
        net::zmq::DealerSocket mMsgSock;
        net::zmq::DealerSocket mRequestSock;
        net::zmq::PairSocket  mDispatchSock;
        suil::UnorderedMap<OnAirMessage::Ptr> mOnAirMessages;
        bool mExiting{false};
        bool mServerConnected{false};
    };


}
