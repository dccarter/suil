/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-17
 */

#include "suil/async/socket.hpp"
#include "suil/async/fdwait.hpp"

#include <unistd.h>
#include <sys/socket.h>

namespace suil {

    Socket::Socket(Socket&& other) noexcept
        : _fd{std::exchange(other._fd, INVALID_FD)},
          _error{std::exchange(other._error, 0)}
    {}

    Socket& Socket::operator=(Socket&& other) noexcept
    {
        if (this != std::addressof(other)) {
            _fd = std::exchange(other._fd, INVALID_FD);
            _error = std::exchange(other._error, 0);
        }
        return *this;
    }

    Socket::~Socket() noexcept
    {
        this->close();
    }

    void Socket::close() noexcept
    {
        if (_fd != INVALID_FD) {
            ::close(_fd);
            _fd = -1;
            _error = 0;
        }
    }

    int Socket::detach()
    {
        return std::exchange(_fd, INVALID_FD);
    }

    auto Socket::send(const void* buf, std::size_t size, milliseconds timeout) -> Task<int>
    {
        auto deadline = after(timeout);
        ssize_t nSent{0};
        do {
            nSent = ::send(_fd, buf, size, MSG_NOSIGNAL);
            if (nSent < 0) {
                if (errno == EPIPE) {
                    _error = errno = ECONNRESET;
                    break;
                }
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    _error = errno;
                    break;
                }

                auto ev = co_await fdwait(_fd, Event::OUT, deadline);
                if (ev != Event::esFIRED) {
                    _error = (ev ==  Event::esTIMEOUT? ETIMEDOUT : errno);
                    break;
                }

                continue;
            }
            break;
        } while (true);

        co_return int(nSent);
    }

    auto Socket::sendn(const void* buf, std::size_t size, milliseconds timeout) -> Task<int>
    {
        auto deadline = after(timeout);
        ssize_t nSent{0}, rc{0};
        do {
            rc = ::send(_fd, &((char  *)buf)[nSent], (size - nSent), MSG_NOSIGNAL);
            if (rc < 0) {
                if (errno == EPIPE) {
                    _error = errno = ECONNRESET;
                    break;
                }
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    _error = errno;
                    nSent = -1;
                    break;
                }

                auto ev = co_await fdwait(_fd, Event::OUT, deadline);
                if (ev != Event::esFIRED) {
                    _error = (ev ==  Event::esTIMEOUT? ETIMEDOUT : errno);
                    nSent = -1;
                    break;
                }

                continue;
            }
            else if (rc == 0) {
                _error = errno = ECONNRESET;
                break;
            }

            nSent += rc;
        } while (nSent < size);

        co_return int(nSent);
    }

    auto Socket::receive(void* buf, std::size_t size, milliseconds timeout) -> Task<int>
    {
        auto deadline = after(timeout);
        ssize_t nReceived{0};
        do {
            nReceived = ::recv(_fd, buf, size, MSG_NOSIGNAL);
            if (nReceived < 0) {
                if (errno == EPIPE) {
                    _error = errno = ECONNRESET;
                    break;
                }
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    _error = errno;
                    break;
                }

                auto ev = co_await fdwait(_fd, Event::IN, deadline);
                if (ev != Event::esFIRED) {
                    _error = (ev ==  Event::esTIMEOUT? ETIMEDOUT : errno);
                    break;
                }

                continue;
            }
            else if (nReceived == 0) {
                _error = errno = ECONNRESET;
            }
            break;
        } while (true);

        co_return int(nReceived);
    }

    auto Socket::recvn(void* buf, std::size_t size, milliseconds timeout) -> Task<int>
    {
        auto deadline = after(timeout);
        ssize_t nReceived{0}, rc{0};
        do {
            rc = ::recv(_fd, &((char *)buf)[nReceived], (size - nReceived), MSG_NOSIGNAL);
            if (rc < 0) {
                if (errno == EPIPE) {
                    _error = errno = ECONNRESET;
                    break;
                }
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    _error = errno;
                    nReceived = -1;
                    break;
                }

                auto ev = co_await fdwait(_fd, Event::IN, deadline);
                if (ev != Event::esFIRED) {
                    _error = (ev ==  Event::esTIMEOUT? ETIMEDOUT : errno);
                    nReceived = -1;
                    break;
                }

                continue;
            }
            else if (rc == 0) {
                _error = errno = ECONNRESET;
                break;
            }

            nReceived += rc;
        } while (nReceived  < size);

        co_return int(nReceived);
    }
}