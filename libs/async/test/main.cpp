//
// Created by Carter on 2022-01-06.
//

#define CATCH_CONFIG_RUNNER
#include <fcntl.h>

#include "catch2/catch.hpp"
#include "suil/async/poll.hpp"
#include "suil/async/tcp.hpp"
#include "suil/async/go.hpp"

using namespace suil;

auto handler(TcpSocket sock) -> Void
{
    printf("new connection: %s:%d\n", sock.address().str().c_str(), sock.address().port());

    while (sock) {
        char buf[1024];
        auto ec = co_await sock.receive({buf, sizeof(buf)}, 5s);
        if (ec > 0) {
            auto size = int(ec - 2);
            printf("\t[%d] -> received: %d %*.s\n", sock.address().port(), size, size, buf);
            ec = co_await sock.send({buf, ec}, 5s);
        }

        if (ec == 0) {
            printf("\t[%d] EoF\n", sock.address().port());
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

auto acceptor(int port = 80) -> AsyncVoid
{
    auto addr = SocketAddress::local("0.0.0.0", port);
    if (!addr) {
        throw std::system_error{errno, std::system_category()};
    }

    auto listener = TcpListener::listen(addr, 127);
    printf("listening on %s:%d\n", addr.str().c_str(), addr.port());

    AsyncScope connectionScope;

    while (listener) {
        auto sock = co_await listener.accept();
        if (!sock) {
            printf("accept failed: %s\n", strerror(errno));
            break;
        }

        connectionScope.spawn(handler(std::move(sock)));
    }

    co_await connectionScope.join();

    Poll::poke(Poll::POKE_STOP);
}

int main(int argc, const char *argv[])
{
    auto accept = acceptor();

    Poll::loop(5s);
    int result = Catch::Session().run(argc, argv);
    return (result < 0xff ? result: 0xff);
}