/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-14
 */

#include "suil/async/poll.hpp"
#include "suil/async/tcp.hpp"
#include "suil/async/go.hpp"

#include <cstring>
#include <cmath>
#include <thread>
#include <iostream>

using namespace suil;
using namespace std::string_literals;

#define SYX_TCP_PORT 8080

#ifdef SYX_TCP_ECHO_SVR

auto handler(TcpSocket sock) -> Void
{
    // printf("new connection: %s:%d\n", sock.address().str().c_str(), sock.address().port());

    while (sock) {
        char buf[1024];
        auto ec = co_await sock.receive({buf, sizeof(buf)}, 10s);
        if (ec > 0) {
            ec = co_await sock.send({buf, std::size_t(ec)}, 10s);
        }

        if (ec == 0) {
            sock.close();
        }
        else if (ec < 0) {
            printf("\t[%d] -> error: %s\n",
                   sock.address().port(),
                   strerror(sock.getLastError()));
            sock.close();
        }
    }
}

auto acceptor(int id, TcpListener& listener) -> AsyncVoid
{

    AsyncScope connectionScope;
    while (listener) {
        auto sock = co_await listener.accept();
        if (!sock) {
            printf("accept failed: %s\n", strerror(errno));
            break;
        }
        connectionScope.spawn(handler(std::move(sock)));
    }

    // join the open utils scope
    co_await connectionScope.join();

    // Poke the poll loop to stop polling (this will immediately exit the application)
    Poll::poke(Poll::POKE_STOP);
}

void multiThreadServer(int tc)
{
    auto listener = TcpListener::listen(SocketAddress::local("0.0.0.0", SYX_TCP_PORT), 127);
    SXY_ASSERT(listener);
    printf("listening on 0.0.0.0:%d\n", SYX_TCP_PORT);

    std::thread ts[tc];
    for (int i = 1; i < tc; i++) {
        ts[i] = std::thread([i, ls = &listener] {
            // Start an acceptor
            auto accept = acceptor(i, *ls);
            // Enter event loop, waking up every 5 seconds if there are no events to handle
            Poll::loop(5s);
        });
    }

    {
        // Start an acceptor
        auto accept = acceptor(0, listener);
        // Enter event loop, waking up every 5 seconds if there are no events to handle
        Poll::loop(5s);
    }

    for (int i = 1; i < tc; i++) {
        if (ts[i].joinable())
            ts[i].join();
    }
}

#else
auto connection(int id, long roundTrips) -> Void
{
    auto conn = co_await TcpSocket::connect(SocketAddress::local("0.0.0.0", SYX_TCP_PORT));
    // printf("%d: connected to server\n", id);
    SXY_ASSERT(conn);

#define MESSAGE     "Hello World"
#define MESSAGE_LEN (sizeof(MESSAGE)-1)

    for (int i = 0; i < roundTrips; ++i) {
        auto ec = co_await conn.send({MESSAGE, MESSAGE_LEN});
        SXY_ASSERT(ec == MESSAGE_LEN);
        char buf[16];
        ec = co_await conn.receive({buf, sizeof(buf)});
        SXY_ASSERT(ec == MESSAGE_LEN);
    }

#undef MESSAGE
#undef MESSAGE_LEN

}

auto client(std::int64_t& duration, long conns, long roundTrips) -> AsyncVoid
{
    AsyncScope connectionsScope;
    printf("starting %ld round trips for %ld connections\n", roundTrips, conns);

    auto start = nowms();
    for (int i = 0; i < conns; ++i) {
        connectionsScope.spawn(connection(i, roundTrips));
    }

    co_await connectionsScope.join();
    duration = nowms() - start;

    // Poke the poll loop to stop polling (this will immediately exit the application)
    Poll::poke(Poll::POKE_STOP);
}

void multiThreadedClients(int tc, long conns, long roundTrips)
{
    std::thread ts[tc];
    std::int64_t durations[tc];
    for (int i = 1; i < tc; i++) {
        durations[i] = 0;
        ts[i] = std::thread([i, conns, roundTrips, d = &durations[i]] {
            auto connector = client(*d, conns, roundTrips);
            Poll::loop(5s);
        });
    }

    {
        auto connector = client(durations[0], conns, roundTrips);
        Poll::loop(5s);
    }

    double duration = double(durations[0]);
    for (int i = 1; i < tc; i++) {
        if (ts[i].joinable())
            ts[i].join();
        duration += double(durations[i]);
    }

    duration /= tc;
    auto requests = tc * conns * roundTrips;
    printf("%ld requests in %.3f ms\n",  requests, duration);
    printf("Performance: %.2f req/s\n", double(requests * 1000)/double(duration));
}

#endif


int main(int argc, const char *argv[])
{
#ifdef SYX_TCP_ECHO_SVR
    constexpr const char* usage = "Usage: syx-tcp-echo-svr [threads:1]\n";
    int tc{1};
    if (argc >= 2) {
        tc = int(strtol(argv[1], nullptr, 10));
        SXY_ASSERT(tc != 0);
    }
    multiThreadServer(tc);
#else
    constexpr const char* usage = "Usage: syx-tcp-echo-cli <conns> <roundtrips> <threads>\n";
    if (argc < 3) {
        printf(usage);
        return EXIT_FAILURE;
    }

    auto conns = strtol(argv[1], nullptr, 10);
    SXY_ASSERT(conns != 0);
    auto roundTrips = strtol(argv[2], nullptr, 10);
    int tc{1};
    if (argc >= 4) {
        tc = int(strtol(argv[3], nullptr, 10));
        SXY_ASSERT(tc != 0);
    }
    multiThreadedClients(tc, conns, roundTrips);
#endif
    return EXIT_SUCCESS;
}