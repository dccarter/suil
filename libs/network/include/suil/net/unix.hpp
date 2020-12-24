//
// Created by Mpho Mbotho on 2020-11-14.
//

#ifndef SUILNETWORK_UNIX_HPP
#define SUILNETWORK_UNIX_HPP

#include "suil/net/socket.hpp"
#include "suil/net/config.scc.hpp"

namespace suil::net {

    define_log_tag(UNIX_SOCK);

    class UnixSocket: public Socket, LOGGER(UNIX_SOCK) {
    public:
        using LOGGER(UNIX_SOCK)::log;
        UnixSocket() = default;
        UnixSocket(unixsock sock);
        DISABLE_COPY(UnixSocket);

        UnixSocket(UnixSocket&& other);
        UnixSocket& operator=(UnixSocket&& other);

        virtual ~UnixSocket();

        const ipaddr addr() const override;
        int port() const override;
        bool connect(
                const String& addr,
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

        static bool pair(UnixSocket& sock1, UnixSocket& sock2);
    private:
        unixsock sock{nullptr};
    };

    class UnixServerSocket: public ServerSocket, LOGGER(UNIX_SOCK) {
    public:
        UnixServerSocket() = default;
        UnixServerSocket(UnixServerSocket& other);
        UnixServerSocket& operator=(UnixServerSocket&& other);

        Socket::UPtr accept(const Deadline &dd = Deadline::infinite()) override;
        bool listen(const String& addr, int backlog) override;
        void close() override;
        void shutdown() override;
        virtual ~UnixServerSocket();

    private:
        unixsock sock{nullptr};
    };
}
#endif //SUILNETWORK_UNIX_HPP
