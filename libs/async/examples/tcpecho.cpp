/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-14
 */

#include "suil/async/scheduler.hpp"
#include "suil/async/tcp.hpp"
#include "suil/async/scope.hpp"
#include "suil/async/sync.hpp"
#include "suil/async/fdwait.hpp"

#include <cstring>
#include <cmath>
#include <thread>
#include <iostream>

using namespace suil;
using namespace std::string_literals;

#define SYX_TCP_PORT 8080

enum State {
    EMPTY,
    CREATED,
    WAIT_READ,
    WAIT_WRITE,
    DONE
};

const char *names[] = {
        "EMPTY",
        "CREATED",
        "WAIT_READ",
        "WAIT_WRITE",
        "DONE",
};

State states[50] = {EMPTY};

void dumpStates()
{
    printf("states:[\n\t%s", names[states[0]]);
    for (int i = 1; i < 20; i++) {
        if (i % 5 == 0)
            printf("\n""\t");
        printf(", %s", names[states[i]]);
    }
    printf("\n]\n");
}

std::unordered_map<size_t, ::size_t> fairness;

#ifdef SYX_TCP_ECHO_SVR

auto handler(int id, TcpSocket sock) -> Task<>
{
    relocate_to(id%6u);
    fairness[tid()]++;
    while (sock) {
        char buf[1024];
        auto ec = co_await sock.receive({buf, sizeof(buf)});
        if (ec > 0) {
            ec = co_await sock.send({buf, std::size_t(ec)}, 10s);
        }

        if (ec == 0) {
            sock.close();
        }
        else if (ec < 0) {
            std::printf("\t[%d] -> error: %s\n", sock.address().port(), strerror(sock.getLastError()));
            sock.close();
        }
    }
}
void _dumpFairness()
{
    auto f = std::move(fairness);
    for (auto& [t, n] : f) {
        printf("[%022zu] %zu\n", t, n);
    }
}

auto dumpFairness() -> Task<>
{
    while (true) {
        delay(20s);
        _dumpFairness();
    }
}

auto acceptor(TcpListener& listener) -> VoidTask<true>
{
    AsyncScope connectionScope;
    connectionScope.spawn(dumpFairness());

    int i = 0;
    while (listener) {
        auto sock = co_await listener.accept();
        if (!sock) {
            printf("accept failed: %s\n", strerror(errno));
            break;
        }

        connectionScope.spawn(handler(i++, std::move(sock)));
    }

    // join the open utils scope
    co_await connectionScope.join();
}

void multiThreadServer()
{
    auto listener = TcpListener::listen(SocketAddress::local("0.0.0.0", SYX_TCP_PORT), 127);
    SUIL_ASSERT(listener);
    printf("listening on 0.0.0.0:%d\n", SYX_TCP_PORT);
    auto ac = acceptor(listener);
    ac.join();
}

#else
auto connection(int id, long roundTrips) -> Task<>
{
    relocate_to(id%6u);
    fairness[tid()]++;
    auto conn = co_await TcpSocket::connect(SocketAddress::local("0.0.0.0", SYX_TCP_PORT));
    SUIL_ASSERT(conn);

#define MESSAGE     "Hello World"
#define MESSAGE_LEN (sizeof(MESSAGE)-1)

    for (int i = 0; i < roundTrips; ++i) {
        auto ec = co_await conn.send({MESSAGE, MESSAGE_LEN}, 10s);
        SUIL_ASSERT(ec == MESSAGE_LEN);
        char buf[16];
        ec = co_await conn.receive({buf, sizeof(buf)}, 10s);
        if (ec != MESSAGE_LEN) {
            break;
        }
    }
#undef MESSAGE
#undef MESSAGE_LEN

}

auto clients(long conns, long roundTrips) -> VoidTask<>
{
    AsyncScope connectionsScope;

    for (int i = 0; i < conns; ++i) {
        connectionsScope.spawn(connection(i, roundTrips));
    }

    co_await connectionsScope.join();
}

void multiThreadedClients(long conns, long roundTrips)
{
    auto start = nowms();

    auto handle = clients(conns, roundTrips);
    handle.join();

    auto duration = nowms() - start;

    auto requests = conns * roundTrips;
    for (auto [t, n] : fairness) {
        printf("[%022zu] %zu\n", t, n);
    }
    printf("%ld requests in %ld ms\n",  requests, duration);
    printf("Performance: %.2f req/s\n", double(requests * 1000)/double(duration));
}

#endif

int main(int argc, const char *argv[])
{
    Scheduler::init();
#ifdef SYX_TCP_ECHO_SVR
    multiThreadServer();
#else
    constexpr const char* usage = "Usage: syx-tcp-echo-cli <conns> <roundtrips>\n";
    if (argc < 3) {
        printf(usage);
        return EXIT_FAILURE;
    }

    auto conns = strtol(argv[1], nullptr, 10);
    SUIL_ASSERT(conns != 0);
    auto roundTrips = strtol(argv[2], nullptr, 10);
    multiThreadedClients(conns, roundTrips);
#endif
    Scheduler::abort();
    return EXIT_SUCCESS;
}