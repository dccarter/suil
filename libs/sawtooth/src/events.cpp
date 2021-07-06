//
// Created by Mpho Mbotho on 2021-06-30.
//


#include <suil/sawtooth/events.hpp>
#include <suil/sawtooth/transaction.hpp>


#include <suil/base/uuid.hpp>

namespace sp {
    using namespace sawtooth::protos;
    constexpr auto CLIENT_EVENTS_SUBSCRIBE_RESPONSE{Message_MessageType_CLIENT_EVENTS_SUBSCRIBE_RESPONSE};
    constexpr auto CLIENT_EVENTS_SUBSCRIBE_REQUEST{Message_MessageType_CLIENT_EVENTS_SUBSCRIBE_REQUEST};
    constexpr auto CLIENT_EVENTS_UNSUBSCRIBE_RESPONSE{Message_MessageType_CLIENT_EVENTS_UNSUBSCRIBE_RESPONSE};
    constexpr auto CLIENT_EVENTS_UNSUBSCRIBE_REQUEST{Message_MessageType_CLIENT_EVENTS_UNSUBSCRIBE_REQUEST};
}

namespace suil::saw {

    static std::string correlationId()
    {
        static std::uint64_t sId;
        return std::to_string(sId++);
    }

    Filter::Filter(String key, String matchString, sp::EventFilter_FilterType filterType)
        : mEventFilter{}
    {
        protoSet(mEventFilter, key, key);
        protoSet(mEventFilter, match_string, matchString);
        protoSet(mEventFilter, filter_type, filterType);
    }

    template <typename Type>
    static void send(net::zmq::DealerSocket& sock, Data payload, const String& tag, Type type)
    {
        sp::Message msg;
        protoSet(msg, message_type, type);
        protoSet(msg, content, payload);
        protoSet(msg, correlation_id, correlationId());

        payload = Data{msg.ByteSizeLong()};
        msg.SerializeToArray(payload.data(), int(payload.size()));

        if (!sock.send(payload)) {
            throw EventSubscriptionError(AnT(),
                                         "Sending ", tag, " request to server failed");
        }
    }

    EventSubscription::EventSubscription(net::zmq::Context& context, String url)
        : mCtx(context),
          mSock(context),
          mId(suil::uuidstr())
    {
        if (!mSock.connect(url)) {
            throw EventSubscriptionError("Connecting to URL '", url, "' failed");
        }
    }

    void EventSubscription::subscribe(const String& eventType, std::vector<Filter> filters)
    {
        if (mHandler) {
            throw EventSubscriptionError(AnT(),
                    "Cannot send subscribe request when waiting for events. (Might support in the future)");
        }

        sp::EventSubscription subscription;
        protoSet(subscription, event_type, eventType);
        for (auto& filter: filters) {
            auto sf = subscription.add_filters();
            sf->CopyFrom(filter());
        }

        sp::ClientEventsSubscribeRequest req;
        req.add_subscriptions()->CopyFrom(subscription);
        suil::Data data{req.ByteSizeLong()};
        req.SerializeToArray(data.data(), static_cast<int>(data.size()));

        sendSubscription(std::move(data));
    }

    void EventSubscription::sendSubscription(suil::Data payload)
    {
        send(mSock,
             std::move(payload),
             "subscribe",
             sp::CLIENT_EVENTS_SUBSCRIBE_REQUEST);

        net::zmq::Message zMsg{};
        if (!mSock.receive(zMsg)) {
            throw EventSubscriptionError(AnT(),
                                         "receiving response to subscribe request failed");
        }

        if (zMsg.empty() || zMsg.back().empty()) {
            throw EventSubscriptionError(AnT(),
                                         "received invalid response to subscribe request");
        }

        auto& frame = zMsg.back();
        sp::Message respMsg;
        respMsg.ParseFromArray(frame.data(), int(frame.size()));
        if (respMsg.message_type() != sp::CLIENT_EVENTS_SUBSCRIBE_RESPONSE) {
            throw EventSubscriptionError(AnT(),
                                         "Unexpected response type (",
                                         respMsg.message_type(),
                                         ") while waiting for CLIENT_EVENTS_SUBSCRIBE_RESPONSE");
        }

        sp::ClientEventsSubscribeResponse resp;
        resp.ParseFromString(respMsg.content());
        if (resp.status() != sp::ClientEventsSubscribeResponse_Status_OK) {
            throw EventSubscriptionError(AnT(),
                                         "subscribe request rejected: ", resp.response_message());
        }
    }

    void EventSubscription::sendCancelSubscription()
    {
        sp::ClientEventsUnsubscribeRequest unsub;
        Data payload{unsub.ByteSizeLong()};
        unsub.SerializeToArray(payload.data(), int(payload.size()));

        send(mSock,
             std::move(payload),
             "unsubscribe", sp::CLIENT_EVENTS_UNSUBSCRIBE_REQUEST);

        // Wait maximum of 2 seconds for response
        if (!mCancelWait.wait(Deadline{2000})) {
            ierror("EventSubscription(" PRIs ") - timed out while waiting for unsubscribe response");
        }
    }

    void EventSubscription::operator()(Handler handler)
    {
        if (!mSock.isConnected() || mHandler) {
            throw EventSubscriptionError(AnT(),
                                 "Current event subscription cannot be started: ",
                                         (mHandler? "already started": "not connected"));
        }
        if (handler == nullptr) {
            throw EventSubscriptionError(AnT(),
                                         "handler cannot be null");
        }
        Ego.mHandler = std::move(handler);
        go(receiveEventsAsync(Ego));
    }

    void EventSubscription::receiveEventsAsync(EventSubscription& S)
    {
        ldebug(&S, "EventSubscription(" PRIs ") - starting receive events async", _PRIs(S.mId));
        while (S.mSock.isConnected()) {
            net::zmq::Message zMsg;
            if (!S.mSock.receive(zMsg) or zMsg.empty() or zMsg.back().empty()) {
                // maybe aborted
                continue;
            }

            auto& frame = zMsg.back();
            sp::Message msg;
            if (!msg.ParseFromArray(frame.data(), int(frame.size()))) {
                lerror(&S, "EventSubscription: parsing received frame failed");
                continue;
            }

            if (msg.message_type() == sp::Message_MessageType_CLIENT_EVENTS_UNSUBSCRIBE_RESPONSE) {
                // unsubscribe was sent to server, break out of loop immediately
                ldebug(&S, "EventSubscription(" PRIs ") - received unsubscribe response, aborting", _PRIs(S.mId));
                S.mCancelWait.notifyOne();
                break;
            }

            if (msg.message_type() != sp::Message_MessageType_CLIENT_EVENTS) {
                lerror(&S, "EventSubscription(" PRIs ") - unexpected message {type: %d}, expecting CLIENT_EVENTS",
                            _PRIs(S.mId), int(msg.message_type()));
                continue;
            }

            sp::EventList eventList;
            eventList.ParseFromString(msg.content());
            for (auto& event: eventList.events()) {
                S.mHandler(event);
            }
        }

        ldebug(&S, "EventSubscription(" PRIs ") - receive events async done ", _PRIs(S.mId));
    }

    void EventSubscription::close()
    {
        if (mSock.isConnected() and mHandler) {
            // send cancellation
            Ego.sendCancelSubscription();
            mSock.close();
        }
    }

    EventSubscription::~EventSubscription()
    {
        close();
    }
}