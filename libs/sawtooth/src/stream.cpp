//
// Created by Mpho Mbotho on 2021-06-04.
//

#include <suil/base/buffer.hpp>

#include "suil/sawtooth/stream.hpp"

namespace suil::saw {

    uint32_t Stream::CorrelationCounter = 0;

    Stream::Stream(net::zmq::Context &ctx, suil::UnorderedMap<OnAirMessage::Ptr> &msgs)
        : mOnAirMsgs{msgs},
          mSocket{ctx}
    {}

    Stream::Stream(Stream&& other) noexcept
            : mOnAirMsgs(other.mOnAirMsgs),
              mSocket(std::move(other.mSocket))
    {}

    Stream& Stream::operator=(Stream &&other) noexcept
    {
        Ego.mOnAirMsgs = other.mOnAirMsgs;
        Ego.mSocket = std::move(other.mSocket);
        return Ego;
    }

    OnAirMessage::Ptr Stream::asyncSend(Message::Type type, const suil::Data &data)
    {
        auto correlationId = Ego.getCorrelationId();
        auto onAir = OnAirMessage::mkshared(std::move(correlationId));
        mOnAirMsgs[onAir->id().peek()] = onAir;
        Ego.send(type, data, onAir->id());
        return onAir;
    }

    void Stream::send(Message::Type type, const suil::Data &data, const suil::String &correlationId)
    {
        if (!Ego.mConnected) {
            if (!Ego.mSocket.connect("inproc://send_queue")) {
                throw StreamError("Failed to connect to send queue: ", zmq_strerror(zmq_errno()));
            }
            Ego.mConnected = true;
        }

        ::sawtooth::protos::Message msg;
        msg.set_message_type(type);
        msg.set_correlation_id(correlationId.data(), correlationId.size());
        msg.set_content(data.cdata(), data.size());
        suil::Data out{msg.ByteSizeLong()};
        msg.SerializeToArray(out.data(), static_cast<int>(out.size()));

        Ego.mSocket.send(out);
    }

    suil::String Stream::getCorrelationId()
    {
        Buffer ob{};
        ob << (++Ego.CorrelationCounter);
        return String{ob};
    }

}