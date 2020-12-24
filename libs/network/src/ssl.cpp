//
// Created by Mpho Mbotho on 2020-11-09.
//

#include "suil/net/ssl.hpp"

#include <sys/mman.h>

namespace suil::net {

    SslSock::SslSock(sslsock sock)
        : sock{sock}
    {}

    SslSock::SslSock(SslSock&& other)
        : sock(other.sock)
    {
        other.sock = nullptr;
    }

    SslSock & SslSock::operator=(SslSock&& other)
    {
        if (this != &other) {
            Ego.sock = other.sock;
            other.sock = nullptr;
        }
        return Ego;
    }

    int SslSock::port() const
    {
        if (sock) {
            return sslport(sock);
        }
        return -1;
    }

    const ipaddr SslSock::addr() const
    {
        if (sock) {
            return ssladdr(sock);
        }
        return ipaddr{};
    }

    bool SslSock::connect(ipaddr addr, const Deadline& dd)
    {
        if (isOpen()) {
            iwarn("connecting an open socket is not supported");
            errno = EINPROGRESS;
            return false;
        }

        sock = sslconnect(addr, dd);
        if (sock == nullptr) {
            itrace("connection to address: %s failed: %s",
                   Socket::ipstr(addr), errno_s);
            return false;
        }

        return true;
    }

    std::size_t SslSock::send(const void *buf, std::size_t len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("writing to a closed socket is not supported");
            errno = ENOTSUP;
            return 0;
        }

        auto ns = sslsend(sock, buf, int(len), dd);
        if (errno) {
            itrace("send error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return 0;
        }
        return ns;
    }

    std::size_t SslSock::sendfile(int fd, off_t offset, std::size_t len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("flushing a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }
        auto addr = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, offset);
        if (addr == MAP_FAILED) {
            iwarn("failed to mmap file for sending over ssl: %s", errno_s);
            return 0;
        }
        defer({
            munmap((void *)addr, len);
        });
        return send(addr, len, dd);
    }

    bool SslSock::flush(const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("flushing a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }

        sslflush(sock, dd);
        if (errno) {
            itrace("ssl flush error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool SslSock::receive(void *dst, std::size_t& len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("receiving from a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }

        len = sslrecv(sock, dst, int(len), dd);
        if (errno) {
            itrace("ssl receive error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool SslSock::receiveUntil(
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

        len = sslrecvuntil(sock, dst, int(len), delims, ndelims, dd);
        if (errno) {
            itrace("ssl receive until error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool SslSock::read(void *dst, std::size_t& len, const Deadline& dd)
    {
        if (!isOpen()) {
            iwarn("reading from a closed socket is not supported");
            errno = ENOTSUP;
            return false;
        }

        len = sslrecv(sock, dst, int(len), dd);
        if (errno) {
            itrace("ssl read error: %s", errno_s);
            if (errno == ECONNRESET) {
                close();
            }
            return false;
        }
        return true;
    }

    bool SslSock::isOpen() const
    {
        return sock != nullptr;
    }

    void SslSock::close()
    {
        if (isOpen()) {
            flush(500);
            sslclose(sock);
            sock = nullptr;
        }
    }

    SslSock::~SslSock() noexcept
    {
        close();
    }

    SslServerSock::SslServerSock(SslSocketConfig config)
        : mConfig{std::move(config)}
    {}

    SslServerSock::~SslServerSock() noexcept
    {
        if (mSock != nullptr) {
            close();
        }
    }

    SslServerSock::SslServerSock(SslServerSock& other)
        : mSock{other.mSock},
          mConfig{std::move(other.mConfig)}
    {
        other.mSock = nullptr;
    }

    SslServerSock& SslServerSock::operator=(SslServerSock&& other)
    {
        if (this != &other) {
            Ego.mConfig = std::move(other.mConfig);
            Ego.mSock = other.mSock;
            other.mSock = nullptr;
        }
        return Ego;
    }

    bool SslServerSock::listen(ipaddr addr, int backlog)
    {
        if (mSock != nullptr) {
            iwarn("server socket already listening");
            errno = EINPROGRESS;
            return false;
        }

        mSock = ssllisten(addr, mConfig.cert.data(), mConfig.cert.data(), backlog);
        if (mSock == nullptr) {
            ierror("listening failed: %s", errno_s);
            return false;
        }
        mRunning = true;
        return true;
    }

    Socket::UPtr SslServerSock::accept(const Deadline& dd)
    {
        auto sock = sslaccept(mSock, dd);
        if (sock == nullptr) {
            itrace("accept connection failed: %s", errno_s);
            return nullptr;
        }
        return std::make_unique<SslSock>(sock);
    }

    void SslServerSock::close()
    {
        if (mSock != nullptr) {
            sslclose(mSock);
            mSock = nullptr;
            mRunning = false;
        }
    }

    void SslServerSock::shutdown()
    {
        if (mSock != nullptr) {
            mRunning = false;
        }
    }
}