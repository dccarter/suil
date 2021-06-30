//
// Created by Mpho Mbotho on 2020-12-08.
//

#ifndef LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_MESSAGE_HPP
#define LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_MESSAGE_HPP

#include <zmq.h>
#include <vector>

#include <suil/base/data.hpp>
#include <suil/base/string.hpp>

namespace suil::net::zmq {

    class Frame {
    public:
        Frame();
        Frame(size_t size, void *src = nullptr);
        Frame(void* src, size_t size, zmq_free_fn *dctor, void *hint = nullptr);

        MOVE_CTOR(Frame) noexcept;
        MOVE_ASSIGN(Frame) noexcept;

        bool isSent() const {
            return _sent;
        }

        bool empty() const {
            return !Ego._valid || Ego.size() == 0;
        }

        const void * data() const;

        void* data();

        size_t size() const;

        bool isValid() const {
            return _valid;
        }

        operator bool() const {
            return isValid();
        }

        const zmq_msg_t& raw() const {
            return _msg;
        }

        zmq_msg_t& raw() {
            return _msg;
        }

        Frame copy() const;

        ~Frame();
    private:
        friend class SendOperator;
        DISABLE_COPY(Frame);
        zmq_msg_t _msg{0};
        bool _sent{false};
        bool _valid{false};
    };

    class Message {
    public:
        using iterator = std::vector<Frame>::iterator;
        using const_iterator = std::vector<Frame>::const_iterator;

        inline iterator       begin()        { return _frames.begin(); }
        inline const_iterator begin() const { return _frames.cbegin(); }
        inline iterator       end()          { return _frames.end(); }
        inline const_iterator end() const   { return _frames.cend(); }

        Message() = default;

        template <typename... Args>
        Frame& emplace(Args&&... args) {
            _frames.emplace_back(std::forward<Args>(args)...);
            return _frames.back();
        }

        void add(Frame&& frame) {
            _frames.push_back(std::move(frame));
        }

        Frame& operator[](size_t index);

        Frame& back();

        bool empty() const {
            return _frames.empty();
        }

        size_t size() const {
            return _frames.size();
        }

        void gather(std::function<void(const suil::Data& data)> func) const;

    private:
        friend struct SendOperator;
        std::vector<Frame> _frames;
    };
}
#endif //LIBS_NETWORK_INCLUDE_SUIL_NET_ZMQ_MESSAGE_HPP
