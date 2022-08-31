//
// Created by Mpho Mbotho on 2020-11-09.
//

#ifndef SUILNETWORK_SOCKET_HPP
#define SUILNETWORK_SOCKET_HPP

#include <suil/base/string.hpp>
#include <suil/base/logging.hpp>

#include <libmill/libmill.h>

namespace suil::net {

    define_log_tag(SOCK_ADAPTOR);

    class Socket : LOGGER(SOCK_ADAPTOR) {
    public:
        sptr(Socket);

        static const char* ipstr(ipaddr addr, char* buf = nullptr);

        virtual bool connect(ipaddr addr, const Deadline& dd = Deadline::infinite());

        virtual bool connect(const String& addr, const Deadline& dd = Deadline::infinite());

        virtual int port() const = 0;

        virtual const ipaddr addr() const = 0;

        virtual std::size_t send(
                const void* buf,
                std::size_t len,
                const Deadline& dd = Deadline::infinite()) = 0;

        virtual std::size_t sendf(const Deadline& dd, const char* fmt, ...);

        virtual std::size_t sendv(const Deadline& dd, const char* fmt, va_list args);

        virtual std::size_t sendfile(
                int fd,
                off_t offset,
                std::size_t len,
                const Deadline& dd = Deadline::infinite()) = 0;

        virtual bool flush(const Deadline& dd = Deadline::infinite()) = 0;

        virtual bool receive(
                void *dst,
                std::size_t& len,
                const Deadline& dd = Deadline::infinite())  = 0;

        virtual bool read(
                void *dst,
                std::size_t& len,
                const Deadline& dd = Deadline::infinite()) = 0;

        virtual bool receiveUntil(
                void *dst,
                std::size_t& len,
                const char* delims,
                std::size_t ndelims,
                const Deadline& dd = Deadline::infinite()) = 0;

        virtual bool isOpen() const = 0;

        virtual void close() = 0;

        virtual void setBuffering(bool on, const Deadline& dd = Deadline::infinite()) {}

        const char *id();

        virtual ~Socket();

    private:
        String mId;
    };

    class ServerSocket {
    public:
        sptr(ServerSocket);
        virtual bool listen(const String& addr, int backlog);
        virtual bool listen(ipaddr addr, int backlog);
        virtual Socket::UPtr accept(const Deadline& dd = Deadline::infinite()) = 0;
        virtual void close() = 0;
        virtual void shutdown() = 0;
        bool isRunning() const { return mRunning; }
        virtual ~ServerSocket() = default;
    protected:
        bool mRunning{false};
    };
}
#endif //SUILNETWORK_SOCKET_HPP
