//
// Created by Mpho Mbotho on 2020-12-09.
//

#include "suil/net/zmq/socket.hpp"
#include "suil/net/zmq/context.hpp"

#include <libmill/libmill.h>

namespace suil::net::zmq {

    Socket::Socket(Context& ctx, Type type)
        : _type{type}
    {
        _raw = zmq_socket(ctx(), type);
        if (_raw == nullptr) {
            iwarn("Socket::(...) zmq_socket failed: %s", zmq_strerror(zmq_errno()));
        }
        createIdentity();
        Option::setLinger(Ego, 0);
    }

    Socket::~Socket() noexcept
    {
        Ego.close();
    }

    bool Socket::set(int opt, const void* buf, size_t len)
    {
        if (_raw == nullptr) {
            idebug("Socket::set(...) invalid socket");
            return false;
        }

        if (0 != zmq_setsockopt(_raw, opt, buf, len)) {
            iwarn("setting ZMQ socket option (%d) failed: %s", opt, zmq_strerror(zmq_errno()));
            return false;
        }
        return true;
    }

    std::pair<bool, size_t> Socket::get(int opt, void* buf, size_t len) const
    {
        if (_raw == nullptr) {
            idebug("Socket::get(...) invalid socket");
            return {false, 0};
        }

        if (0 != zmq_getsockopt(_raw, opt, buf, &len)) {
            iwarn("getting ZMQ socket option (%d) failed: %s", opt, zmq_strerror(zmq_errno()));
            return {false, 0};
        }

        return {true, len};
    }

    int Socket::fd()
    {
        if (unlikely(_raw == nullptr)) {
            idebug("Socket::get(...) invalid socket");
            return -1;
        }
        else {
            if (unlikely(_fd == -1)) {
                if (!Option::getFileDescriptor(Ego, _fd)) {
                    // failed to get underlying file descriptor
                    throw ZmqInternalError("Failed to retrieve socket file descriptor");
                }
            }
            return _fd;
        }
    }

    void Socket::createIdentity()
    {
        if (unlikely(_raw == nullptr)) {
            return;
        }
        else {
            if (_id.empty()) {
                _id = suil::randbytes(16);
                Option::setIdentity(Ego, _id);
                itrace("Created socket identity %s", _id());
            }
        }
    }

    Socket::operator bool() const
    {
        return _raw != nullptr;
    }

    void Socket::close()
    {
        if (_raw != nullptr) {
            if (0 != zmq_close(_raw)) {
                throw ZmqInternalError("zmq_close(", _id, ") failed: ", zmq_strerror(zmq_errno()));
            }

            if (_fd != -1) {
                // abort all event handles from lib mill
                fdclear(_fd);
            }

            _raw = nullptr;
            _fd = -1;
            _type = Inval;
        }
    }

    Socket::Socket(Socket&& other) noexcept
        : _raw{other._raw},
          _type{other._type},
          _id{std::move(other._id)},
          _fd{other._fd}
    {
        other._raw = nullptr;
        other._fd = -1;
        other._type = Inval;
    }

    Socket& Socket::operator=(Socket&& other) noexcept
    {
        _raw = other._raw;
        _type = other._type;
        _id = std::move(other._id);
        _fd = other._fd;
        other._raw = nullptr;
        other._fd = -1;
        other._type = Inval;
        return Ego;
    }
}