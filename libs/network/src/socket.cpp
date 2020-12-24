//
// Created by Mpho Mbotho on 2020-11-09.
//

#include "suil/net/socket.hpp"
#include <suil/base/exception.hpp>

namespace suil::net {

Socket::~Socket() noexcept
{
    itrace("destroying socket %s", Ego.id());
}

const char * Socket::ipstr(ipaddr addr, char *buf)
{
    if (buf == nullptr) {
        static char Addr[IPADDR_MAXSTRLEN];
        return ipaddrstr(addr, Addr);
    }
    return ipstr(addr, buf);
}

const char * Socket::id() {
    if (mId.empty()) {
        char buf[IPADDR_MAXSTRLEN+8];
        snprintf(buf, sizeof(buf), "%s::%d", ipstr(addr()), port());
        mId = String{buf}.dup();
    }
    return mId();
}


bool Socket::connect(ipaddr addr, const Deadline& dd)
{
    throw suil::UnsupportedOperation("Socket::connect(ipaddr) is not supported by current socket type");
}

bool Socket::connect(const String& addr, const Deadline& dd)
{
    throw suil::UnsupportedOperation("Socket::connect(String) is not supported by current socket type");
}


std::size_t Socket::sendv(const Deadline& dd, const char *fmt, va_list args)
{
    char buf[1024] = {0};
    ssize_t sz = vsnprintf(buf, sizeof(buf)-1, fmt, args);
    if (sz < 0) {
        // buffer larger than 1024
        errno = ENOMEM;
        return 0;
    }
    return send(buf, sz, dd);
}

std::size_t Socket::sendf(const Deadline& dd, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    auto ret = sendv(dd, fmt, args);
    va_end(args);
    return ret;

}

bool ServerSocket::listen(ipaddr addr, int backlog)
{
    throw suil::UnsupportedOperation("ServerSocket::listen(ipaddr) is not supported by current socket type");
}

bool ServerSocket::listen(const String& addr, int backlog)
{
    throw suil::UnsupportedOperation("ServerSocket::listen(String) is not supported by current socket type");
}

}

#ifdef SUIL_UNITTEST

#endif