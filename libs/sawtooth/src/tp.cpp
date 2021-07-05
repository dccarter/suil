//
// Created by Mpho Mbotho on 2021-06-04.
//

#include "suil/sawtooth/tp.hpp"

#include <suil/base/syson.hpp>

namespace sp {
    using sawtooth::protos::Message;
    using sawtooth::protos::TransactionHeader;
    using sawtooth::protos::TpRegisterRequest;
    using sawtooth::protos::TpRegisterResponse;
    using sawtooth::protos::TpUnregisterRequest;
    using sawtooth::protos::TpUnregisterResponse;
    using sawtooth::protos::TpProcessResponse;
    using sawtooth::protos::TpProcessRequest;
}

namespace pl {
    using namespace std::placeholders;
}

namespace suil::saw {

    TransactionProcessor::TransactionProcessor(suil::String &&connString)
        : mConnString(connString.dup()),
          mDispatcher{mContext},
          mRespStream{mDispatcher.createStream()},
          mConnMonitor{mContext}
    {}

    void TransactionProcessor::registerAll()
    {
        for(const auto& [fn, handler]: Ego.mHandlers) {
            idebug("registerAll - registering handler %s", fn());
            auto versions = handler->getVersions();
            for(const auto& version: versions) {
                idebug("registerAll - register handler {name: %s, version: %s}", fn(), version());
                sp::TpRegisterRequest req;
                sp::TpRegisterResponse resp;
                setValue(req, &sp::TpRegisterRequest::set_family, fn);
                setValue(req, &sp::TpRegisterRequest::set_version, version);
                setValue(req, &sp::TpRegisterRequest::set_protocol_version, uint32(FeatureVersion::FeatureUnused));
                setValue(req, &sp::TpRegisterRequest::set_max_occupancy, handler->getMaxOccupancy());

                for (const auto& ns: handler->getNamespaces()) {
                    setValue(req, &sp::TpRegisterRequest::add_namespaces, ns);
                }

                setValue(req, &sp::TpRegisterRequest::set_request_header_style, Ego.mHeaderStyle);

                auto future = Ego.mRespStream.asyncSend(sp::Message::TP_REGISTER_REQUEST, req);
                future->getMessage(resp, sp::Message::TP_REGISTER_RESPONSE);
                if (resp.status() != sp::TpRegisterResponse::OK) {
                    throw TransactionProcessorError("failed to register handler {name: ",
                                            fn, ", version: ", version, "}");
                }
                idebug("registerAll - handler successfully registered {protocol: %u}", resp.protocol_version());
            }
        }
    }

    void TransactionProcessor::unRegisterAll()
    {
        sp::TpUnregisterRequest req;
        sp::TpUnregisterResponse resp;

        auto future = Ego.mRespStream.asyncSend(sp::Message::TP_UNREGISTER_REQUEST, req);
        future->getMessage(resp, sp::Message::TP_UNREGISTER_RESPONSE);

        if (resp.status() != sp::TpUnregisterResponse::OK) {
            ierror("TpUnregisterRequest failed: ", resp.status());
        }
        else {
            idebug("TpUnregisterRequest ok");
        }
    }

    void TransactionProcessor::handleRequest(const suil::Data &msg, const suil::String &cid)
    {
        sp::TpProcessRequest req;
        sp::TpProcessResponse resp;
        try {
            req.ParseFromArray(msg.cdata(), static_cast<int>(msg.size()));
            sp::TransactionHeader* txnHeader{req.release_header()};
            suil::String fn{txnHeader->family_name()};
            auto it = Ego.mHandlers.find(fn);
            if (it != Ego.mHandlers.end()) {
                try {
                    Transaction txn(TransactionHeader{txnHeader}, req.payload(), req.signature());
                    // WORK AROUND, ignore work around noop transaction
                    GlobalState gs{mDispatcher.createStream(), String{req.context_id()}.dup()};
                    auto applicator = it->second->getProcessor(std::move(txn), std::move(gs));
                    try {
                        applicator->apply();
                        resp.set_status(sp::TpProcessResponse::OK);
                    }
                    catch (InvalidTransaction& ex) {
                        ierror("InvalidTransaction error: %s", ex.what());
                        resp.set_status(sp::TpProcessResponse::INVALID_TRANSACTION);
                    }
                    catch (...) {
                        auto ex = Exception::fromCurrent();
                        ierror("Transaction apply error: %s", ex.what());
                        resp.set_status(sp::TpProcessResponse::INTERNAL_ERROR);
                    }
                }
                catch (...) {
                    auto ex = Exception::fromCurrent();
                    ierror("Handle request error: %s", ex.what());
                    resp.set_status(sp::TpProcessResponse::INTERNAL_ERROR);
                }
            }
            else {
                resp.set_status(sp::TpProcessResponse::INVALID_TRANSACTION);
            }
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            ierror("Transaction process error: %s", ex.what());
            resp.set_status(sp::TpProcessResponse::INTERNAL_ERROR);
        }

        Ego.mRespStream.sendResponse(sp::Message::TP_PROCESS_RESPONSE, resp, cid);
    }

    void TransactionProcessor::initConnectionMonitor()
    {
        Ego.mConnMonitor.onEvent += [&](int16 ev, int32 value, const String& addr) {
            itrace("connectionMonitor received events %02X", ev);
            if (ev & ZMQ_EVENT_CONNECTED) {
                idebug("server connected event");
                if (!Ego.mIsSeverConnected) {
                    Ego.registerAll();
                }
                Ego.mIsSeverConnected = true;
            }
            else if (ev & ZMQ_EVENT_DISCONNECTED) {
                idebug("server disconnected event");
                Ego.mIsSeverConnected = false;
            }
            else {
                idebug("connectionMonitor ignoring connection events %02X", ev);
            }
        };
    }

    void TransactionProcessor::run()
    {
        Once sOnceExit;
        On({SIGNAL_TERM, SIGNAL_QUIT, SIGNAL_INT},
           [&](auto p1, auto p2, auto p3) { handleExitSignal(p1, p2, p3); },
           sOnceExit);

        try {
            net::zmq::DealerSocket sock(Ego.mContext);
            sock.connect("inproc://request_queue");
            Ego.mDispatcher.connect(Ego.mConnString);
            Ego.mConnMonitor.start(
                    Ego.mDispatcher.mServerSock,
                    Dispatcher::SERVER_MONITOR_ENDPOINT, ZMQ_EVENT_CONNECTED|ZMQ_EVENT_DISCONNECTED);

            Ego.mRunning = true;
            Ego.initConnectionMonitor();

            while (Ego.mRunning) {
                net::zmq::Message msg;
                if (!sock.receive(msg) || msg.empty() || msg.back().empty()) {
                    continue;
                }

                sp::Message validatorMsg;
                auto& frame = msg.back();
                validatorMsg.ParseFromArray(frame.data(), static_cast<int>(frame.size()));

                switch (validatorMsg.message_type()) {
                    case sp::Message::TP_PROCESS_REQUEST: {
                        Ego.handleRequest(fromStdString(validatorMsg.content()), String{validatorMsg.correlation_id()});
                        break;
                    }
                    default: {
                        idebug("Unknown message in transaction processor: %08X", validatorMsg.message_type());
                        break;
                    }
                }
            }
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            ierror("Unexpected error while running TP: %s", ex.what());
        }

        idebug("TP done, unregistering");
        Ego.unRegisterAll();
        Ego.mDispatcher.exit();
    }

    void TransactionProcessor::handleExitSignal(int sig, siginfo_t* /*info*/, void* /*ctx*/)
    {
        idebug("transaction processor received exit signal %d", sig);
        if (Ego.mIsSeverConnected) {
            Ego.unRegisterAll();
        }
    }

}