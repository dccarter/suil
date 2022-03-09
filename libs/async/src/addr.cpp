/**
 * Copyright (c) 2022 suilteam, Carter
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author Mpho Mbotho
 * @date 2022-01-09
 */

#include "suil/async/addr.hpp"
#include "suil/async/fdwait.hpp"

#include "dns/dns.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <system_error>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/un.h>

namespace suil {
    static struct dns_resolv_conf *suil_dns_conf{nullptr};
    static struct dns_hosts *suil_dns_hosts{nullptr};
    static struct dns_hints *suil_dns_hints{nullptr};

    SocketAddress SocketAddress::ipv4FromLiteral(const char *addr, int port)
    {
        SocketAddress ipAddress{};
        auto ipv4 = (struct sockaddr_in*) ipAddress._data;
        int rc = inet_pton(AF_INET, addr, &ipv4->sin_addr);
        if (rc < 0) {
            throw std::system_error{errno, std::system_category(), "inet_pton(AF_INET)"};
        }

        if(rc == 1) {
            ipv4->sin_family = AF_INET;
            ipv4->sin_port = htons((uint16_t)port);
            errno = 0;
            return ipAddress;
        }

        ipv4->sin_family = AF_UNSPEC;
        errno = EINVAL;
        return ipAddress;
    }

    SocketAddress SocketAddress::ipv6FromLiteral(const char *addr, int port)
    {
        SocketAddress  ipAddr{};
        auto ipv6 = (struct sockaddr_in6*) ipAddr._data;
        int rc = inet_pton(AF_INET6, addr, &ipv6->sin6_addr);
        if (rc < 0) {
            throw std::system_error{errno, std::system_category(), "inet_pton(AF_INET6)"};
        }

        if(rc == 1) {
            ipv6->sin6_family = AF_INET6;
            ipv6->sin6_port = htons((uint16_t)port);
            errno = 0;
            return ipAddr;
        }
        ipv6->sin6_family = AF_UNSPEC;
        errno = EINVAL;
        return ipAddr;
    }

    SocketAddress SocketAddress::ipFromLiteral(const char *addr, int port, suil::SocketAddress::Mode mode)
    {
        SocketAddress ipAddr{};
        auto sa = (struct sockaddr*) ipAddr._data;
        if(unlikely(!addr || port < 0 || port > 0xffff)) {
            sa->sa_family = AF_UNSPEC;
            errno = EINVAL;
            return ipAddr;
        }

        switch(mode) {
            case suil::SocketAddress::IPV4:
                return ipv4FromLiteral(addr, port);
            case suil::SocketAddress::IPV6:
                return ipv6FromLiteral(addr, port);
            case suil::SocketAddress::IP_ANY:
            case suil::SocketAddress::IP_PV4:
                ipAddr = ipv4FromLiteral(addr, port);
                if(errno == 0)
                    return ipAddr;
                return ipv6FromLiteral(addr, port);
            case suil::SocketAddress::IP_PV6:
                ipAddr = ipv6FromLiteral(addr, port);
                if(errno == 0)
                    return ipAddr;
                return ipv4FromLiteral(addr, port);
            default:
                throw std::runtime_error{"ipFromLiteral(unsupported mode)"};
        }
    }

    SocketAddress SocketAddress::any(int port, Mode mode)
    {
        SocketAddress addr{};
        if(unlikely(port < 0 || port > 0xffff)) {
            ((struct sockaddr*) addr._data)->sa_family = AF_UNSPEC;
            errno = EINVAL;
            return addr;
        }
        if (mode == IP_ANY || mode == IPV4 || mode == IP_PV4) {
            auto ipv4 = (struct sockaddr_in*) addr._data;
            ipv4->sin_family = AF_INET;
            ipv4->sin_addr.s_addr = htonl(INADDR_ANY);
            ipv4->sin_port = htons((uint16_t)port);
        }
        else {
            auto ipv6 = (struct sockaddr_in6*) addr._data;
            ipv6->sin6_family = AF_INET6;
            memcpy(&ipv6->sin6_addr, &in6addr_any, sizeof(in6addr_any));
            ipv6->sin6_port = htons((uint16_t)port);
        }
        errno = 0;
        return addr;
    }

    int SocketAddress::family() const
    {
        return ((struct sockaddr*)_data)->sa_family;
    }

    int SocketAddress::size() const
    {
        return family() == AF_INET?
               sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
    }

    int SocketAddress::port() const
    {
        return ntohs(family() == AF_INET?
               ((struct sockaddr_in*)_data)->sin_port :
               ((struct sockaddr_in6*)_data)->sin6_port);
    }

    SocketAddress::operator bool() const
    {
        return family() != AF_UNSPEC;
    }

    std::string SocketAddress::str() const
    {
        if (family() == AF_INET) {
            char buf[INET_ADDRSTRLEN];
            return inet_ntop(AF_INET, &(((struct sockaddr_in*)_data)->sin_addr), buf, INET_ADDRSTRLEN);
        }
        else if (family() == AF_INET6) {
            char buf[INET6_ADDRSTRLEN];
            return inet_ntop(AF_INET6, &(((struct sockaddr_in6*)_data)->sin6_addr), buf, INET6_ADDRSTRLEN);
        }
        return "<invalid>";
    }

    SocketAddress SocketAddress::local(const std::string &name, int port, Mode mode)
    {
        if (name.empty()) {
            return any(port, mode);
        }

        SocketAddress addr = ipFromLiteral(name.data(), port, mode);
        if (errno == 0) {
            return addr;
        }

        /* Address is not a literal. It must be an interface name then. */
        struct ifaddrs *ifaces = nullptr;
        int rc = getifaddrs (&ifaces);
        if (rc < 0) {
            throw std::system_error{errno, std::system_category(), "getifaddrs(...)"};
        }
        if (ifaces == nullptr) {
            throw std::runtime_error{"getifaddrs returned 0 interfaces"};
        }

        /*  Find first IPv4 and first IPv6 address. */
        struct ifaddrs *ipv4 = nullptr;
        struct ifaddrs *ipv6 = nullptr;
        struct ifaddrs *it;
        for(it = ifaces; it != nullptr; it = it->ifa_next) {
            if(!it->ifa_addr)
                continue;
            if(strcmp(it->ifa_name, name.data()) != 0)
                continue;
            switch(it->ifa_addr->sa_family) {
                case AF_INET:
                    SUIL_ASSERT(!ipv4);
                    ipv4 = it;
                    break;
                case AF_INET6:
                    SUIL_ASSERT(!ipv6);
                    ipv6 = it;
                    break;
            }
            if(ipv4 && ipv6)
                break;
        }
        /* Choose the correct address family based on mode. */
        switch(mode) {
            case IPV4:
                ipv6 = nullptr;
                break;
            case IPV6:
                ipv4 = nullptr;
                break;
            case 0:
            case IP_PV4:
                if(ipv4)
                    ipv6 = nullptr;
                break;
            case IP_PV6:
                if(ipv6)
                    ipv4 = nullptr;
                break;
            default:
                SUIL_ASSERT(0);
        }

        if(ipv4) {
            auto inaddr = (struct sockaddr_in*) addr._data;
            memcpy(inaddr, ipv4->ifa_addr, sizeof (struct sockaddr_in));
            inaddr->sin_port = htons(port);
            freeifaddrs(ifaces);
            errno = 0;
            return addr;
        }

        if(ipv6) {
            auto inaddr = (struct sockaddr_in6*) addr._data;
            memcpy(inaddr, ipv6->ifa_addr, sizeof (struct sockaddr_in6));
            inaddr->sin6_port = htons(port);
            freeifaddrs(ifaces);
            errno = 0;
            return addr;
        }

        freeifaddrs(ifaces);
        ((struct sockaddr*) addr._data)->sa_family = AF_UNSPEC;
        errno = ENODEV;
        return addr;
    }

    JoinableTask<SocketAddress> SocketAddress::remote(const std::string &name, int port, Mode mode, milliseconds timeout)
    {
        int rc;
        SocketAddress addr = ipFromLiteral(name.data(), port, mode);
        if(errno == 0)
            co_return addr;

        /* Load DNS config files, unless they are already chached. */
        if(unlikely(!suil_dns_conf)) {
            suil_dns_conf = dns_resconf_local(&rc);
            assert(suil_dns_conf);
            suil_dns_hosts = dns_hosts_local(&rc);
            assert(suil_dns_hosts);
            suil_dns_hints = dns_hints_local(suil_dns_conf, &rc);
            SUIL_ASSERT(suil_dns_hints);
        }
        /* Let's do asynchronous DNS query here. */
        auto opts = dns_opts();
        struct dns_resolver *resolver = dns_res_open(suil_dns_conf, suil_dns_hosts,
                                                     suil_dns_hints, nullptr, &opts, &rc);
        SUIL_ASSERT(resolver);
        SUIL_ASSERT(port >= 0 && port <= 0xffff);
        char portstr[8];
        snprintf(portstr, sizeof(portstr), "%d", port);
        struct addrinfo hints{};
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = PF_UNSPEC;
        struct dns_addrinfo *ai = dns_ai_open(name.data(), portstr, DNS_T_A, &hints, resolver, &rc);
        SUIL_ASSERT(ai);
        dns_res_close(resolver);

        struct addrinfo *ipv4 = nullptr;
        struct addrinfo *ipv6 = nullptr;
        struct addrinfo *it = nullptr;
        auto deadline = after(timeout);

        while (true) {
            rc = dns_ai_nextent(&it, ai);
            if (rc == EAGAIN) {
                int fd = dns_ai_pollfd(ai);
                SUIL_ASSERT(fd >= 0);
                auto fdw = co_await fdwait(fd, Event::IN, deadline);
                if (fdw != Event::esFIRED) {
                    co_return addr;
                }

                continue;
            }
            if (rc == ENOENT)
                break;

            if (!ipv4 && it && it->ai_family == AF_INET) {
                ipv4 = it;
            }
            else if (!ipv6 && it && it->ai_family == AF_INET6) {
                ipv6 = it;
            }
            else {
                free(it);
            }

            if(ipv4 && ipv6)
                break;
        }

        switch(mode) {
            case IPV4:
                if (ipv6) {
                    free(ipv6);
                    ipv6 = nullptr;
                }
                break;
            case IPV6:
                if(ipv4) {
                    free (ipv4);
                    ipv4 = nullptr;
                }
                break;
            case 0:
            case IP_PV4:
                if (ipv4 && ipv6) {
                    free(ipv6);
                    ipv6 = nullptr;
                }
                break;
            case IP_PV6:
                if (ipv6 && ipv4) {
                    free(ipv4);
                    ipv4 = nullptr;
                }
                break;
            default:
                SUIL_ASSERT(0);
        }

        if  (ipv4) {
            auto inaddr = (struct sockaddr_in*) addr._data;
            memcpy(inaddr, ipv4->ai_addr, sizeof (struct sockaddr_in));
            inaddr->sin_port = htons(port);
            dns_ai_close(ai);
            free(ipv4);
            errno = 0;
            co_return addr;
        }

        if (ipv6) {
            auto inaddr = (struct sockaddr_in6*) addr._data;
            memcpy(inaddr, ipv6->ai_addr, sizeof (struct sockaddr_in6));
            inaddr->sin6_port = htons(port);
            dns_ai_close(ai);
            free(ipv6);
            errno = 0;
            co_return addr;
        }

        dns_ai_close(ai);
        ((struct sockaddr*) addr._data)->sa_family = AF_UNSPEC;
        errno = EADDRNOTAVAIL;
        co_return addr;
    }
}
