//
// Created by Mpho Mbotho on 2021-06-04.
//

#include "suil/sawtooth/dispatcher.hpp"

namespace suil::saw {

    const suil::String Dispatcher::DISPATCH_THREAD_ENDPOINT{"inproc://dispatch_thread"};
    const suil::String Dispatcher::SERVER_MONITOR_ENDPOINT{"inproc://server_monitor"};
    const suil::String Dispatcher::EXIT_MESSAGE{"Exit"};

    Dispatcher::Dispatcher(net::zmq::Context& ctx)
        : mContext{ctx},
          mServerSock{mContext},
          mMsgSock{mContext},
          mRequestSock{mContext},
          mDispatchSock{mContext}
    {
        Ego.mMsgSock.bind("inproc://send_queue");
        Ego.mRequestSock.bind("inproc://request_queue");
        Ego.mDispatchSock.bind(DISPATCH_THREAD_ENDPOINT);
    }

    Stream Dispatcher::createStream()
    {
        return Stream{Ego.mContext, Ego.mOnAirMessages};
    }

    void Dispatcher::connect(const suil::String &connString)
    {
        iinfo("Dispatcher connecting to %s", connString());
        bool status{false};
        try {
            status = Ego.mServerSock.connect(connString);
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            ierror("Connecting to server failed: %s", ex.what());
            throw;
        }

        if (!status) {
            ierror("Connecting to server failed: %s", zmq_strerror(zmq_errno()));
            throw DispatcherError("Connecting to server failed: ", zmq_strerror(zmq_errno()));
        }

        go(sendMessages(Ego));
        go(exitMonitor(Ego));
        go(receiveMessages(Ego));
    }

    void Dispatcher::disconnect()
    {
        iinfo("Disconnecting server socket");
        Ego.mServerSock.close();
    }

    void Dispatcher::exit()
    {
        idebug("dispatcher exiting...");
        Ego.mDispatchSock.send(Dispatcher::EXIT_MESSAGE);
    }

    void Dispatcher::receiveMessages(Dispatcher &Self)
    {
        ldebug(&Self, "starting receiveMessages coroutine");
        while (!Self.mExiting) {
            net::zmq::Message zmsg{};
            if (!Self.mServerSock.receive(zmsg) || zmsg.empty() || zmsg.back().empty()) {
                ldebug(&Self, "receivedMessages - ignoring empty message");
                continue;
            }

            Message::UPtr proto(Message::mkunique());
            auto& frame = zmsg.back();
            proto->ParseFromArray(frame.data(), static_cast<int>(frame.size()));
            ltrace(&Self, "received message {type: %d}", proto->message_type());

            switch (proto->message_type()) {
                case sawtooth::protos::Message_MessageType_TP_PROCESS_REQUEST: {
                    Self.mRequestSock.send(zmsg);
                    break;
                }
                case sawtooth::protos::Message_MessageType_PING_REQUEST: {
                    ltrace(&Self, "Received ping request with correlation %s", proto->correlation_id().data());
                    sawtooth::protos::PingResponse resp;
                    suil::Data data{resp.ByteSizeLong()};
                    resp.SerializeToArray(data.data(), static_cast<int>(data.size()));

                    sawtooth::protos::Message msg;
                    msg.set_correlation_id(proto->correlation_id());
                    msg.set_message_type(sawtooth::protos::Message_MessageType_PING_RESPONSE);
                    msg.set_content(data.cdata(), data.size());
                    suil::Data rdata{msg.ByteSizeLong()};
                    msg.SerializeToArray(rdata.data(), static_cast<int>(rdata.size()));
                    Self.mServerSock.send(rdata);
                    break;
                }
                default: {
                    String cid{proto->correlation_id(), false};
                    auto it = Self.mOnAirMessages.find(cid);
                    if (it != Self.mOnAirMessages.end()) {
                        it->second->setMessage(std::move(proto));
                        Self.mOnAirMessages.erase(it);
                    }
                    else {
                        ldebug(&Self, "Received a message with no matching correlation %s", cid());
                    }
                }
            }
        }
        ldebug(&Self, "receive messages coroutine exit");
    }

    void Dispatcher::sendMessages(Dispatcher &Self)
    {
        ldebug(&Self, "Starting sendMessages coroutine");
        while (!Self.mExiting) {
            net::zmq::Message msg{};
            if (!Self.mMsgSock.receive(msg) || msg.empty()) {
                ldebug(&Self, "sendMessages - ignoring empty message");
                continue;
            }

            if (!Self.mServerSock.isConnected()) {
                ldebug(&Self, "sendMessages - server is not connected, dropping message");
                continue;
            }

            Self.mServerSock.send(msg);
        }
        ldebug(&Self, "Exiting sendMessages coroutines");
    }

    void Dispatcher::exitMonitor(Dispatcher &Self)
    {
        ldebug(&Self, "Starting exitMonitor coroutine");
        net::zmq::PairSocket sock(Self.mContext);
        sock.connect(Dispatcher::DISPATCH_THREAD_ENDPOINT);

        while (!Self.mExiting) {
            net::zmq::Message msg{};
            if (!sock.receive(msg) || msg.empty() || msg.back().empty()) {
                Self.mExiting = true;
                continue;
            }
            String what{static_cast<const char*>(msg.back().data()), msg.back().size(), false};
            Self.mExiting = (what == EXIT_MESSAGE);
        }
        Self.mServerSock.close();
        Self.mMsgSock.close();
        ldebug(&Self, "Exiting exitMonitor coroutine");
    }

}