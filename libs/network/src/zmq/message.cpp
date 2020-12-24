//
// Created by Mpho Mbotho on 2020-12-08.
//

#include "suil/net/zmq/message.hpp"
#include "suil/net/zmq/common.hpp"

namespace suil::net::zmq {

    LOGGER(ZMQ_SOCK) ZmqLog;

    Frame::Frame()
        : Frame(0)
    {}

    Frame::Frame(size_t size, void* src)
    {
        if (size == 0) {
            if (0 != zmq_msg_init(&_msg)) {
                // message is invalid
                ldebug(&ZmqLog, "()::zmq_msg_init(...) failed: %s", zmq_strerror(zmq_errno()));
                return;
            }
        }
        else {
            if (0 != zmq_msg_init_size(&_msg, size)) {
                ldebug(&ZmqLog, "()::zmq_msg_init_size(...) failed: %s", zmq_strerror(zmq_errno()));
                return;
            }

            if (src != nullptr) {
                auto* d = zmq_msg_data(&_msg);
                memcpy(d, src, size);
            }
        }
        _valid = true;
    }

    Frame::Frame(void* src, size_t size, zmq_free_fn* dctor, void* hint)
    {
        if (src == nullptr or size == 0 or dctor == nullptr) {
            ldebug(&ZmqLog, "(%s, %zu, %s, ?) invalid args",
                   (src? "ok": "null"),
                   size,
                   (dctor? "ok": "null"));
            return;
        }

        if (0 != zmq_msg_init_data(&_msg, src, size, dctor, hint)) {
            ldebug(&ZmqLog, "()::zmq_msg_init_data(...) failed: %s", zmq_strerror(zmq_errno()));
            return;
        }
        _valid = true;
    }

    Frame::Frame(Frame&& other) noexcept
        : _sent{other._sent},
          _valid{other._valid}
    {
        other._valid = false;
        if (!Ego._valid) {
            // message invalid, nothing to do
            return;
        }

        if (0 != zmq_msg_init(&_msg)) {
            ldebug(&ZmqLog, "(&&)::zmq_msg_init(...) failed: %s", zmq_strerror(zmq_errno()));
            Ego._valid = false;
            return;
        }

        if (0 != zmq_msg_move(&_msg, &other._msg)) {
            Ego._valid = false;
            ldebug(&ZmqLog, "(&&)::zmq_msg_move(...) failed: %s", zmq_strerror(zmq_errno()));
            return;
        }
    }

    Frame& Frame::operator=(Frame&& other) noexcept
    {
        std::swap(_sent, other._sent);
        std::swap(_valid, other._valid);
        if (!Ego._valid) {
            // can't move an invalid message
            return Ego;
        }

        if (0 != zmq_msg_init(&_msg)) {
            ldebug(&ZmqLog, "(&&)::zmq_msg_init(...) failed: %s", zmq_strerror(zmq_errno()));
            Ego._valid = false;
            return Ego;
        }

        if (0 != zmq_msg_move(&_msg, &other._msg)) {
            ldebug(&ZmqLog, "(&&)::zmq_msg_move(...) failed: %s", zmq_strerror(zmq_errno()));
            Ego._valid = false;
        }
        return Ego;
    }

    Frame Frame::copy() const
    {
        if (!Ego._valid) {
            Frame frame{};
            frame._valid = false;
            return frame;
        }

        Frame frame(size());
        if (frame && (0 != zmq_msg_copy(&frame._msg, const_cast<zmq_msg_t *>(&Ego._msg)))) {
            ldebug(&ZmqLog, "(&&)::zmq_msg_init(...) failed: %s", zmq_strerror(zmq_errno()));
            frame._valid = false;
        }
        return frame;
    }

    Frame::~Frame()
    {
        // always attempt to close the message
        zmq_msg_close(&Ego._msg);
        Ego._valid = false;
    }

    void* Frame::data()
    {
        if (Ego._valid) {
            return zmq_msg_data(&Ego._msg);
        }
        return nullptr;
    }

    const void* Frame::data() const
    {
        if (Ego._valid) {
            return zmq_msg_data(const_cast<zmq_msg_t *>(&Ego._msg));
        }
        return nullptr;
    }

    size_t Frame::size() const
    {
        if (Ego._valid) {
            return zmq_msg_size(const_cast<zmq_msg_t *>(&Ego._msg));
        }
        return 0;
    }

    Frame& Message::operator[](size_t index)
    {
        if (index >= _frames.size()) {
            throw IndexOutOfBounds();
        }
        return _frames[index];
    }

    Frame& Message::back()
    {
        if (_frames.empty()) {
            throw UnsupportedOperation("Message is empty");
        }
        return _frames.back();
    }

    void Message::gather(std::function<void(const suil::Data&)> func) const
    {
        for (auto& frame: _frames) {
            suil::Data tmp{frame.data(), frame.size(), false};
            func(tmp);
        }
    }
}