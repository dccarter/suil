//
// Created by Mpho Mbotho on 2020-11-14.
//

#include "suil/net/unix.hpp"

namespace suil::net {

    UnixSocket::UnixSocket(unixsock sock)
        : sock(sock)
    {}

    UnixSocket::UnixSocket(UnixSocket&& other)
        : sock{other.sock}
    {
        other.sock = nullptr;
    }

    UnixSocket & UnixSocket::operator=(UnixSocket&& other)
    {
        if (this != &other) {
            sock = other.sock;
            other.sock = nullptr;
        }
        return Ego;
    }

    int UnixSocket::port() const
    {
        return -1;
    }

    const ipaddr UnixSocket::addr() const
    {
        return ipaddr{};
    }

    bool UnixSocket::connect(const String& addr, const Deadline& dd)
    {
        if (isOpen()) {
            iwarn("connecting an open socket is not supported");
            errno = EINPROGRESS;
            return false;
        }

        sock = unixconnect(addr());
        if (sock == nullptr) {
            itrace("connection to address: %s failed: %s",
                   Socketet::ipstr(addr), errno_s);
            return false;
        }

        return true;
    }

    std::size_t UnixSocket::send(const void *buf, std::size_t len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("writing to a closed socket is not supported");
            errno = ENOTSUP;
            return 0;
        }

        auto ns = unixsend(sock, buf, int(len), dd);
        if (errno) {
            itrace("send error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return 0;
        }
        return ns;
    }

    std::size_t UnixSocket::sendfile(int fd, off_t offset, std::size_t len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("writing to a closed socket is not supported");
            errno = ENOTSUP;
            return 0;
        }
        errno = ENOTSUP;
        return 0;
#if 0
        auto ns = unixsendfile(sock, fd, offset, int(len), dd);
        if (errno) {
            itrace("sendfile error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return 0;
        }
        return ns;
#endif
    }

    bool UnixSocket::flush(const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("flushing a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }

        unixflush(sock, dd);
        if (errno) {
            itrace("tcp flush error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool UnixSocket::receive(void *dst, std::size_t& len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("receiving from a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }

        len = unixrecv(sock, dst, int(len), dd);
        if (errno) {
            idebug("tcp receive error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool UnixSocket::receiveUntil(
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

        len = unixrecvuntil(sock, dst, int(len), delims, ndelims, dd);
        if (errno) {
            itrace("tcp receive until error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool UnixSocket::read(void *dst, std::size_t& len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("reading from a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }

        len = unixrecv(sock, dst, int(len), dd);
        if (errno) {
            itrace("tcp read error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool UnixSocket::isOpen() const
    {
        return sock != nullptr;
    }

    void UnixSocket::close()
    {
        if (isOpen()) {
            flush(500);
            unixclose(sock);
            sock = nullptr;
        }
    }

    bool UnixSocket::pair(UnixSocket& sock1, UnixSocket& sock2)
    {
        if (sock1.isOpen()) {
            lerror(&sock1, "Socket sock1 is already open, cannot be paired");
            errno = EINPROGRESS;
            return false;
        }

        if (sock2.isOpen()) {
            lerror(&sock2, "Socket sock2 is already open, cannot be paired");
            errno = EINPROGRESS;
            return false;
        }

        unixpair(&sock1.sock, &sock2.sock);
        if (!sock1.isOpen() or sock2.isOpen()) {
            lerror(&sock1, "Creating unix socket pair failed: %s", errno_s);
            return false;
        }

        return true;
    }

    UnixSocket::~UnixSocket() noexcept
    {
        close();
    }

    UnixServerSocket::~UnixServerSocket() noexcept
    {
        if (sock != nullptr) {
            close();
        }
    }

    UnixServerSocket::UnixServerSocket(UnixServerSocket& other)
            : sock{other.sock}
    {
        other.sock = nullptr;
        other.mRunning = false;
    }

    UnixServerSocket& UnixServerSocket::operator=(UnixServerSocket&& other)
    {
        if (this != &other) {
            Ego.sock = other.sock;
            other.sock = nullptr;
            other.mRunning = false;
        }
        return Ego;
    }

    bool UnixServerSocket::listen(const String& addr, int backlog)
    {
        if (sock != nullptr) {
            iwarn("server socket already listening");
            errno = EINPROGRESS;
            return false;
        }

        sock = unixlisten(addr(), backlog);
        if (sock == nullptr) {
            ierror("listening failed: %s", errno_s);
            return false;
        }
        mRunning  = true;
        return true;
    }

    Socket::UPtr UnixServerSocket::accept(const Deadline& dd)
    {
        if (sock == nullptr) {
            iwarn("server socket is not listening");
            errno = ECONNREFUSED;
            return nullptr;
        }

        auto client = unixaccept(sock, dd);
        if (client == nullptr) {
            itrace("accept connection failed: %s", errno_s);
            return nullptr;
        }
        return std::make_unique<UnixSocket>(client);
    }

    void UnixServerSocket::close()
    {
        if (sock != nullptr) {
            unixclose(sock);
            sock = nullptr;
            mRunning = false;
        }
    }

    void UnixServerSocket::shutdown()
    {
        if (sock) {
            unixshutdown(sock, 2);
            mRunning = false;
        }
    }
}