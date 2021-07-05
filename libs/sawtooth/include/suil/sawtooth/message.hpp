//
// Created by Mpho Mbotho on 2021-05-07.
//

#pragma once

#include <suil/base/exception.hpp>
#include <suil/base/utils.hpp>
#include <suil/base/string.hpp>
#include <suil/sawtooth/protos.hpp>

#include <libmill/libmill.hpp>

namespace suil::saw {

    struct Message {
        using Type = ::sawtooth::protos::Message::MessageType;
        sptr(::sawtooth::protos::Message);
    };

    DECLARE_EXCEPTION(UnexpectedMessage);

    class OnAirMessage final {
    public:
        sptr(OnAirMessage);

        OnAirMessage(suil::String id);

        OnAirMessage(OnAirMessage&& other) noexcept;

        OnAirMessage&operator=(OnAirMessage&& other) noexcept;

        OnAirMessage(const OnAirMessage&) = delete;
        OnAirMessage&operator=(const OnAirMessage&) = delete;

        operator bool() const {
            return Ego.mMessage != nullptr;
        }

        [[nodiscard]]
        const suil::String& id() const { return mCorrelationId; }

        template <typename T>
        void getMessage(T& proto, Message::Type type) {
            if (Ego.mMessage == nullptr) {
                waitForMessage(type);
            }

            const auto& data = mMessage->content();
            proto.ParseFromArray(data.c_str(), data.size());
        }

        void setMessage(Message::UPtr&& message);

        ~OnAirMessage();

    private:
        void waitForMessage(Message::Type type);
        Message::UPtr mMessage{nullptr};
        suil::String mCorrelationId{};
        mill::Event mSync{};
    };
}
