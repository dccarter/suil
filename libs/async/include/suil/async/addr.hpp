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

#include <cstdint>
#include <string>
#include <chrono>

#include "suil/async/task.hpp"

namespace suil {

    struct SocketAddress {
        static constexpr int MAX_IP_ADDRESS_SIZE{32u};

        typedef enum {
            IP_ANY,
            IPV4,
            IPV6,
            IP_PV4,
            IP_PV6
        } Mode;

        int family() const;
        int size() const;
        int port() const;
        const void* const raw() const { return _data; }
        std::string str() const;
        operator bool() const;

        static SocketAddress any(int port, Mode mode = IP_PV4);
        static SocketAddress local(const std::string& name, int port, Mode mode = IP_PV4);
        static JoinableTask<SocketAddress> remote(
                const std::string& name,
                int port,
                Mode mode = IP_PV4,
                milliseconds timeout = DELAY_INF);

        static JoinableTask<SocketAddress> remote(
                const std::string& name,
                int port,
                milliseconds timeout)
        {
            return remote(name, port, IP_PV4, timeout);
        }

    private:
        friend struct TcpListener;
        friend struct TcpSocket;

        static SocketAddress ipv4FromLiteral(const char *addr, int port);
        static SocketAddress ipv6FromLiteral(const char *addr, int port);
        static SocketAddress ipFromLiteral(const char *addr, int port, suil::SocketAddress::Mode mode);
        char _data[MAX_IP_ADDRESS_SIZE];
    };
}