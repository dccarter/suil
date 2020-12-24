//
// Created by Mpho Mbotho on 2020-11-10.
//

#ifndef SUILNETWORK_TCP_HPP
#define SUILNETWORK_TCP_HPP

#include "suil/net/socket.hpp"
#include "suil/net/config.scc.hpp"

namespace suil::net {

    define_log_tag(TCP_SOCK);

    class TcpSock : public virtual  Socket, LOGGER(TCP_SOCK) {
    public:
        using LOGGER(TCP_SOCK)::log;

        TcpSock() = default;
        TcpSock(tcpsock sock);

        DISABLE_COPY(TcpSock);

        TcpSock(TcpSock&& other);
        TcpSock& operator=(TcpSock&& other);

        virtual ~TcpSock();

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
        void setBuffering(bool on, const Deadline &dd = Deadline::infinite()) override;

        bool isOpen() const override;
        void close() override;
    private:
        tcpsock sock{nullptr};
    };

    class TcpServerSock : public virtual ServerSocket, LOGGER(TCP_SOCK) {
    public:
        TcpServerSock() = default;

        DISABLE_COPY(TcpServerSock);

        TcpServerSock(TcpServerSock& other);
        TcpServerSock& operator=(TcpServerSock&& other);

        Socket::UPtr accept(const Deadline &dd = Deadline::infinite()) override;
        bool listen(ipaddr addr, int backlog) override;
        void close() override;
        void shutdown() override;
        virtual ~TcpServerSock();
    private:
        tcpsock sock{nullptr};
    };
}

#endif //SUILNETWORK_TCP_HPP
