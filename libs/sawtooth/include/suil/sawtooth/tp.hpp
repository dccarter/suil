//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/net/zmq/context.hpp>
#include <suil/sawtooth/dispatcher.hpp>
#include <suil/sawtooth/transaction.hpp>
#include <suil/sawtooth/gstate.hpp>
#include <suil/sawtooth/processor.hpp>

#include <suil/net/zmq/monitor.hpp>

namespace suil::saw {

    define_log_tag(SAWSDK_TP);

    DECLARE_EXCEPTION(TransactionProcessorError);

    using TpRequestHeaderStyle = sawtooth::protos::TpRegisterRequest_TpProcessRequestHeaderStyle;

    enum FeatureVersion : std::uint32_t  {
        FeatureUnused = 0,
        FeatureCustomHeaderStyle = 1,
        SdkProtocolVersion = 1
    };

    struct TransactionProcessor : LOGGER(SAWSDK_TP) {
    public:
            static constexpr auto HeaderStyleUnset{TpRequestHeaderStyle::TpRegisterRequest_TpProcessRequestHeaderStyle_HEADER_STYLE_UNSET};
            static constexpr auto HeaderStyleExpanded{TpRequestHeaderStyle::TpRegisterRequest_TpProcessRequestHeaderStyle_EXPANDED};
            static constexpr auto HeaderStyleRaw{TpRequestHeaderStyle::TpRegisterRequest_TpProcessRequestHeaderStyle_RAW};

        TransactionProcessor(suil::String&& connString);

        TransactionProcessor(TransactionProcessor&&) = delete;
        TransactionProcessor(const TransactionProcessor&) = delete;
        TransactionProcessor&operator=(TransactionProcessor&&) = delete;
        TransactionProcessor&operator=(const TransactionProcessor&) = delete;

        net::zmq::Context& zmqContext() { return mContext; }

        void run();

        template <typename Handler>
        TransactionHandlerBase& registerHandler(const String& family, const String& ns)
        {
            idebug("registering transaction handler " PRIs, _PRIs(family));
            Ego.mHandlers[family.dup()] = TransactionHandlerBase::UPtr{new Handler(family, ns)};
            return *Ego.mHandlers[family];
        }

        void setHeaderStyle(TpRequestHeaderStyle headerStyle) { Ego.mHeaderStyle = headerStyle; }

    private:
        void registerAll();
        void unRegisterAll();
        void handleRequest(const suil::Data& msg, const suil::String& cid);
        void initConnectionMonitor();
        void handleExitSignal(int, siginfo_t*, void*);

        net::zmq::Context mContext{};
        net::zmq::SocketMonitor mConnMonitor;
        String mConnString{};
        Dispatcher mDispatcher;
        Stream mRespStream;
        UnorderedMap<TransactionHandlerBase::UPtr> mHandlers;
        bool mRunning{false};
        bool mIsSeverConnected{false};
        TpRequestHeaderStyle mHeaderStyle{HeaderStyleUnset};
    };

}