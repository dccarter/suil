//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/net/zmq/socket.hpp>
#include <suil/net/zmq/patterns.hpp>

#include <suil/sawtooth/message.hpp>

namespace suil::saw {

    DECLARE_EXCEPTION(StreamError);

    struct Stream {
        sptr(Stream);

        Stream(Stream&& other) noexcept;
        Stream&operator=(Stream&& other) noexcept;

        Stream(const Stream&) = delete;
        Stream&operator=(const Stream&) = delete;

        template <typename T>
        OnAirMessage::Ptr asyncSend(Message::Type type, const T& msg) {
            suil::Data data{msg.ByteSizeLong()};
            msg.SerializeToArray(data.data(), data.size());
            return Ego.asyncSend(type, data);
        }

        OnAirMessage::Ptr asyncSend(Message::Type type, const suil::Data& data);

        template <typename T>
        void sendResponse(Message::Type type, const T& msg, const suil::String& correlationId) {
            suil::Data data{msg.ByteSizeLong()};
            msg.SerializeToArray(data.data(), data.size());
            Ego.send(type, data, correlationId);
        }

        void send(Message::Type type, const suil::Data& data, const suil::String& correlationId);

    private:
        friend struct Dispatcher;
        friend struct TpContext;
        Stream(net::zmq::Context& ctx, suil::UnorderedMap<OnAirMessage::Ptr>& msgs);

        suil::String getCorrelationId();
        net::zmq::PushSocket mSocket;
        static uint32_t CorrelationCounter;
        suil::UnorderedMap<OnAirMessage::Ptr>& mOnAirMsgs;
        std::atomic_uint16_t mConnected{false};
    };

}
