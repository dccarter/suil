//
// Created by Mpho Mbotho on 2020-11-10.
//

#include "suil/net/tcp.hpp"

namespace suil::net {

    TcpSock::TcpSock(tcpsock sock)
            : sock{sock}
    {}

    TcpSock::TcpSock(TcpSock&& other)
            : sock(other.sock)
    {
        other.sock = nullptr;
    }

    TcpSock & TcpSock::operator=(TcpSock&& other)
    {
        if (this != &other) {
            Ego.sock = other.sock;
            other.sock = nullptr;
        }
        return Ego;
    }

    int TcpSock::port() const
    {
        if (sock) {
            return tcpport(sock);
        }
        return -1;
    }

    const ipaddr TcpSock::addr() const
    {
        if (sock) {
            return tcpaddr(sock);
        }
        return ipaddr{};
    }

    bool TcpSock::connect(ipaddr addr, const Deadline& dd)
    {
        if (isOpen()) {
            iwarn("connecting an open socket is not supported");
            errno = EINPROGRESS;
            return false;
        }

        sock = tcpconnect(addr, dd);
        if (sock == nullptr) {
            itrace("connection to address: %s failed: %s",
                   Socket::ipstr(addr), errno_s);
            return false;
        }

        return true;
    }

    std::size_t TcpSock::send(const void *buf, std::size_t len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("writing to a closed socket is not supported");
            errno = ENOTSUP;
            return 0;
        }

        auto ns = tcpsend(sock, buf, int(len), dd);
        if (errno) {
            itrace("send error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return 0;
        }
        return ns;
    }

    std::size_t TcpSock::sendfile(int fd, off_t offset, std::size_t len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("writing to a closed socket is not supported");
            errno = ENOTSUP;
            return 0;
        }

        auto ns = tcpsendfile(sock, fd, offset, int(len), dd);
        if (errno) {
            itrace("sendfile error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return 0;
        }
        return ns;
    }

    bool TcpSock::flush(const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("flushing a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }

        tcpflush(sock, dd);
        if (errno) {
            itrace("tcp flush error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool TcpSock::receive(void *dst, std::size_t& len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("receiving from a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }

        len = tcprecv(sock, dst, int(len), dd);
        if (errno) {
            itrace("tcp receive error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool TcpSock::receiveUntil(
            void *dst,
            std::size_t& len,
            const char *delims,
            std::size_t ndelims,
            const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("receiving from a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }

        len = tcprecvuntil(sock, dst, int(len), delims, ndelims, dd);
        if (errno) {
            itrace("tcp receive until error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool TcpSock::read(void *dst, std::size_t& len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("reading from a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }

        len = tcpread(sock, dst, int(len), dd);
        if (errno) {
            if (len != 0 and errno == EAGAIN)
                return true;
            itrace("tcp read error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    void TcpSock::setBuffering(bool on, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("setting buffering on a closed socket is not supported");
            errno = ENOTSUP;
            return;
        }
        tcpbuffering(sock, int(on), dd);
    }

    bool TcpSock::isOpen() const
    {
        return sock != nullptr;
    }

    void TcpSock::close()
    {
        if (isOpen()) {
            flush(500);
            tcpclose(sock);
            sock = nullptr;
        }
    }

    TcpSock::~TcpSock() noexcept
    {
        close();
    }

    TcpServerSock::~TcpServerSock() noexcept
    {
        if (sock != nullptr) {
            close();
        }
    }

    TcpServerSock::TcpServerSock(TcpServerSock& other)
            : sock{other.sock}
    {
        other.sock = nullptr;
        other.mRunning = false;
    }

    TcpServerSock& TcpServerSock::operator=(TcpServerSock&& other)
    {
        if (this != &other) {
            Ego.sock = other.sock;
            other.sock = nullptr;
            other.mRunning = false;
        }
        return Ego;
    }

    bool TcpServerSock::listen(ipaddr addr, int backlog)
    {
        if (sock != nullptr) {
            iwarn("server socket already listening");
            errno = EINPROGRESS;
            return false;
        }

        sock = tcplisten(addr, backlog);
        if (sock == nullptr) {
            ierror("listening failed: %s", errno_s);
            return false;
        }
        mRunning  = true;
        return true;
    }

    Socket::UPtr TcpServerSock::accept(const Deadline& dd)
    {
        if (sock == nullptr) {
            iwarn("server socket is not listening");
            errno = ECONNREFUSED;
            return nullptr;
        }

        auto client = tcpaccept(sock, dd);
        if (client == nullptr) {
            itrace("accept connection failed: %s", errno_s);
            return nullptr;
        }
        return std::make_unique<TcpSock>(client);
    }

    void TcpServerSock::close()
    {
        if (sock != nullptr) {
            tcpclose(sock);
            sock = nullptr;
            mRunning = false;
        }
    }

    void TcpServerSock::shutdown()
    {
        if (sock) {
            tcpshutdown(sock, 2);
            mRunning = false;
        }
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

using suil::net::TcpServerSock;
using suil::net::TcpSock;
using suil::net::Socket;

char buffer[1024];

static coroutine void handleSocket(Socket::UPtr sock) {
    while (sock->isOpen()) {
        size_t len{1023};
        sock->receive(buffer, len);
        sock->send(buffer, len);
    }
}


static coroutine void accept(TcpServerSock::Ptr& server)
{
    auto s = server;
    while (s->isRunning()) {
        if (auto sock = s->accept()) {
            go(handleSocket(std::move(sock)));
        }
    }
}

TEST_CASE("Using TCP Client/Server", "[net][tcp]")
{
    auto addr = iplocal("127.0.0.1", 8889, 0);
    WHEN("Launching a server") {
        TcpServerSock sock;
        auto status = sock.listen(iplocal("127.0.0.1", 8888, 0), 3);
        REQUIRE(status);
        sock.close();
    }

    SECTION("Interacting with a TCP server") {
        TcpServerSock::Ptr server{new TcpServerSock};
        auto status = server->listen(addr, 3);
        REQUIRE(status);

        go(accept(server));

         WHEN("When connecting to a server") {
            TcpSock s1, s2, s3, s4;
            // following connection's should succeed
            status = s1.connect(addr, 500);
            REQUIRE(status);
            status = s2.connect(addr, 500);
            REQUIRE(status);
            status = s3.connect(addr, 500);
            REQUIRE(status);
            status = s4.connect(addr, 500);
            REQUIRE(status);
            // Cannot connect an already connected socket
            status = s4.connect(addr, 500);
            REQUIRE_FALSE(status);
        }

        server->close();
    }
}

#endif