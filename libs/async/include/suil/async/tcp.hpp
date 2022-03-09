/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-01-09
 */

#pragma once

#include <suil/async/addr.hpp>
#include <suil/async/socket.hpp>

#include <span>

namespace suil {

    class TcpSocket : public Socket {
    public:
        TcpSocket() noexcept = default;
        TcpSocket(int fd, int err = 0) : Socket(fd, err)
        {}

        virtual ~TcpSocket() noexcept = default;

        TcpSocket(TcpSocket&& other) noexcept;
        TcpSocket& operator=(TcpSocket&& other) noexcept;

        TcpSocket(const TcpSocket&) = delete;
        TcpSocket& operator=(const TcpSocket&) = delete;

        static Task<TcpSocket> connect(const SocketAddress& addr, milliseconds timeout = DELAY_INF);

        operator bool() const { return isValid(); }

        const SocketAddress& address() const { return _address; }

        void close() noexcept override;
        int detach() override;

        friend class TcpListener;
        SocketAddress _address{};
    };

    class TcpListener {
    public:
        ~TcpListener() noexcept;

        TcpListener(TcpListener&& other) noexcept;
        TcpListener& operator=(TcpListener&& other) noexcept;

        TcpListener(const TcpListener&) = delete;
        TcpListener& operator=(const TcpListener&) = delete;

        operator bool() const { return _fd > 0; }

        auto accept(milliseconds timeout = DELAY_INF) -> Task<TcpSocket>;
        void close();
        int getLastError() const { return _error; }

        static TcpListener listen(const SocketAddress& addr, int backlog);

    private:
        TcpListener() = default;
        TcpListener(int fd, int port, int err = -1) : _fd{fd}, _port{port}, _error{err}
        {}

        int _fd{INVALID_FD};
        int _port{0};
        int _error{0};
    };

    inline auto tcpconnect(const SocketAddress& addr, std::chrono::milliseconds timeout = DELAY_INF) {
        return TcpSocket::connect(addr, timeout);
    }

    inline auto tcplisten(const SocketAddress& addr, int backlog) {
        return TcpListener::listen(addr, backlog);
    }
}