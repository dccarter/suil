//
// Created by Mpho Mbotho on 2020-11-09.
//

#ifndef SUILNETWORK_SSL_HPP
#define SUILNETWORK_SSL_HPP

#include "suil/net/socket.hpp"
#include "suil/net/config.scc.hpp"

namespace suil::net {

    define_log_tag(SSL_SOCK);

    class SslSock : public virtual  Socket, LOGGER(SSL_SOCK) {
    public:
        using LOGGER(SSL_SOCK)::log;

        SslSock() = default;
        SslSock(sslsock sock);

        DISABLE_COPY(SslSock);

        SslSock(SslSock&& other);
        SslSock& operator=(SslSock&& other);

        virtual ~SslSock();

        const ipaddr addr() const override;
        int port() const override;
        bool connect(
                ipaddr addr,
                const Deadline &dd = Deadline::infinite()) override;
        std::size_t send(
                const void *buf,
                std::size_t len,
                const Deadline &dd = Deadline::infinite()) override;
        std::size_t sendfile(
                int fd,
                off_t offset,
                std::size_t len,
                const Deadline &dd = Deadline::infinite()) override;
        bool flush(const Deadline &dd = Deadline::infinite()) override;
        bool receive(
                void *dst,
                std::size_t &len,
                const Deadline &dd = Deadline::infinite()) override;
        bool receiveUntil(
                void *dst,
                std::size_t& len,
                const char* delims,
                std::size_t ndelims,
                const Deadline& dd = Deadline::infinite()) override;
        bool read(
                void *dst,
                std::size_t &len,
                const Deadline &dd = Deadline::infinite()) override;

        bool isOpen() const override;
        void close() override;
    private:
        sslsock sock{nullptr};
    };

    class SslServerSock : public virtual ServerSocket, LOGGER(SSL_SOCK) {
    public:
        SslServerSock() = default;
        explicit SslServerSock(SslSocketConfig config);

        DISABLE_COPY(SslServerSock);

        SslServerSock(SslServerSock& other);
        SslServerSock& operator=(SslServerSock&& other);

        Socket::UPtr accept(const Deadline &dd = Deadline::infinite()) override;
        bool listen(ipaddr addr, int backlog) override;
        void close() override;
        void shutdown() override;
        virtual ~SslServerSock();
    private:
        sslsock mSock{nullptr};
        SslSocketConfig mConfig;
    };
}
#endif //SUILNETWORK_SSL_HPP
