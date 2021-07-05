//
// Created by Mpho Mbotho on 2021-05-07.
//

#include "suil/sawtooth/message.hpp"

namespace suil::saw {

    OnAirMessage::OnAirMessage(suil::String cid)
        : mCorrelationId(std::move(cid))
    {}

    OnAirMessage::OnAirMessage(OnAirMessage&& other) noexcept
        : mMessage{std::move(other.mMessage)},
          mCorrelationId{std::move(other.mCorrelationId)},
          mSync{std::move(other.mSync)}
    {}

    OnAirMessage& OnAirMessage::operator=(OnAirMessage&& other) noexcept
    {
        if (this != &other) {
            mCorrelationId = std::move(other.mCorrelationId);
            mSync = std::move(other.mSync);
            mMessage = std::move(other.mMessage);
        }
        return Ego;
    }

    OnAirMessage::~OnAirMessage()
    {
        // notify waiters if any
        Ego.mSync.notify();
    }

    void OnAirMessage::waitForMessage(Message::Type type)
    {
        if (mMessage == nullptr) {
            mSync.wait();
            if (mMessage == nullptr) {
                throw UnexpectedMessage("No message was received");
            }
        }

        if (mMessage->message_type() != type) {
            throw UnexpectedMessage("Unexpected response message type, expecting: ",
                                    type, ", got: ", mMessage->message_type());
        }
    }

    void OnAirMessage::setMessage(Message::UPtr&& message)
    {
        Ego.mMessage = std::move(message);
        mSync.notify();
    }
}