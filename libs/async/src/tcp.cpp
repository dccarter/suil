/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-01-09
 */

#include "suil/async/tcp.hpp"
#include "suil/async/fdwait.hpp"

#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace {

    void tcptune(int s) noexcept
    {
        SXY_ASSERT(s >= 0);

        int opt = fcntl(s, F_GETFL, 0);
        if (opt == -1) {
            opt = 0;
        }

        int rc = fcntl(s, F_SETFL, opt | O_NONBLOCK);
        SXY_ASSERT(rc != -1);

        //  allow re-using the same local address rapidly
        opt = 1;
        rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
        SXY_ASSERT(rc == 0);
        // if possible, prevent SIGPIPE signal when writing to the connection
        // already closed by the peer
#ifdef SO_NOSIGPIPE
        opt = 1;
        rc = setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof (opt));
        SXY_ASSERT (rc == 0 || errno == EINVAL);
#endif
    }

}

namespace suil {

    TcpSocket::TcpSocket(TcpSocket &&other) noexcept
        : Socket(std::move(other)),
          _address{std::move(other._address)}
    {}

    TcpSocket &TcpSocket::operator=(TcpSocket &&other) noexcept
    {
        if (this != std::addressof(other)) {
            Socket::operator=(std::move(other));
            _address = std::move(other._address);
        }
        return *this;
    }

    Task<TcpSocket> TcpSocket::connect(const SocketAddress& addr, milliseconds timeout)
    {
        int s = socket(addr.family(), SOCK_STREAM, 0);
        if (s == -1) {
            co_return TcpSocket{s, errno };
        }

        tcptune(s);

        int rc = ::connect(s, (struct sockaddr*) addr._data, addr.size());

        if (rc != 0) {
            SXY_ASSERT(rc == -1);
            if(errno != EINPROGRESS) {
                co_return TcpSocket{s, errno};
            }

            auto fdw = co_await fdwait(s, FDW_OUT, after(timeout));
            if(fdw == FDW_TIMEOUT || fdw == FDW_ERR) {
                co_return TcpSocket{s, errno};
            }
            SXY_ASSERT(fdw == FDW_OUT);

            int err;
            socklen_t errsz = sizeof(err);
            rc = getsockopt(s, SOL_SOCKET, SO_ERROR, (void*)&err, &errsz);
            if(rc != 0) {
                err = errno;
                ::close(s);
                errno = err;
                co_return TcpSocket{s, errno};
            }

            if(err != 0) {
                ::close(s);
                errno = err;
                co_return TcpSocket{s, errno};
            }
        }

        errno = 0;
        co_return TcpSocket{s};
    }

    void TcpSocket::close() noexcept
    {
        if (isValid()) {
            Socket::close();
            _address = {};
        }
    }

    int TcpSocket::detach()
    {
        if (isValid()) {
            _address = {};
        }
        return Socket::detach();
    }

    TcpListener::~TcpListener() noexcept
    {
        this->close();
    }

    TcpListener::TcpListener(TcpListener &&other) noexcept
        : _fd{std::exchange(other._fd, INVALID_FD)},
          _port{std::exchange(other._port, 0)}
    {}

    TcpListener &TcpListener::operator=(TcpListener &&other) noexcept
    {
        if (this != std::addressof(other)) {
            _fd = std::exchange(other._fd, INVALID_FD);
            _port = std::exchange(other._port, 0);
        }
        return *this;
    }

    void TcpListener::close()
    {
        if (_fd != INVALID_FD) {
            ::shutdown(_fd, SHUT_RDWR);
            _fd = INVALID_FD;
            _port = 0;
        }
    }

    auto TcpListener::accept(std::chrono::milliseconds timeout) -> Task<TcpSocket>
    {
        socklen_t addrlen;
        TcpSocket sock{};
        auto dd = after(timeout);

        while (true) {
            addrlen = SocketAddress::MAX_IP_ADDRESS_SIZE;
            int as = ::accept(_fd, (struct sockaddr *) sock._address._data, &addrlen);
            if (as >= 0) {
                tcptune(as);
                sock._fd = as;
                _error = errno = 0;
                break;
            }

            SXY_ASSERT(as == -1);
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                _error = errno;
                break;
            }

            auto rc = co_await  fdwait(_fd, FDW_IN, dd);
            if (rc == FDW_TIMEOUT || rc == FDW_ERR) {
                _error = errno;
                break;
            }
            SXY_ASSERT(rc == FDW_IN);
        }

        co_return sock;
    }

    TcpListener TcpListener::listen(const SocketAddress &addr, int backlog)
    {
        int s = socket(addr.family(), SOCK_STREAM, 0);
        if (s == -1) {
            return {s, 0, -1};
        }
        tcptune(s);

        int rc = bind(s, (struct sockaddr*) addr.raw(), addr.size());
        if (rc != 0) {
            ::close(rc);
            return {-1, 0, errno};
        }

        rc = ::listen(s, backlog);
        if (rc != 0) {
            ::close(rc);
            return {-1, 0, errno};
        }

        // if the user requested an ephemeral port,
        // retrieve the port number assigned by the OS now
        auto port = addr.port();
        if (port == 0) {
            SocketAddress ipAddress{};
            socklen_t len = SocketAddress::MAX_IP_ADDRESS_SIZE;
            rc = getsockname(s, (struct sockaddr*) ipAddress._data, &len);
            if (rc == -1) {
                int err = errno;
                ::close(s);
                errno = err;
                return {-1, 0, err};
            }
            port = ipAddress.port();
        }

        errno = 0;
        return TcpListener{s, port};
    }
}