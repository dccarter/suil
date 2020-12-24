//
// Created by Mpho Mbotho on 2020-11-12.
//

#include "suil/net/server.hpp"
#include "suil/net/ssl.hpp"
#include "suil/net/tcp.hpp"
#include "suil/net/unix.hpp"

namespace suil::net {

    ServerSocket::UPtr createAdaptor(const SocketConfig& config)
    {
        if (config.has<SslSocketConfig>()) {
            auto sslConfig = (const SslSocketConfig &) config;
            return std::make_unique<SslServerSock>(sslConfig);
        }
        else if (config.has<TcpSocketConfig>()){
            return std::make_unique<TcpServerSock>();
        }
        else {
            return std::make_unique<UnixServerSocket>();
        }
    }

    bool adaptorListen(ServerSocket& adaptor, const SocketConfig& config, int backlog)
    {
        if (config.has<TcpSocketConfig>()) {
            auto& tcpConfig = (const TcpSocketConfig &) config;
            return adaptor.listen(iplocal(tcpConfig.bindAddr.name(), tcpConfig.bindAddr.port, 0), backlog);
        }
        else if (config.has<SslSocketConfig>()) {
            auto sslConfig = (const SslSocketConfig &) config;
            return adaptor.listen(iplocal(sslConfig.bindAddr.name(), sslConfig.bindAddr.port, 0), backlog);
        }
        else {
            auto& unixConfig = (const UnixSocketConfig &) config;
            return adaptor.listen(unixConfig.bindAddr, backlog);
        }
    }

    String getAddress(const SocketConfig& config)
    {
        if (config.has<TcpSocketConfig>()) {
            auto& tcpConfig = (const TcpSocketConfig &) config;
            return suil::catstr(tcpConfig.bindAddr.name, ":", tcpConfig.bindAddr.port);
        }
        else if (config.has<SslSocketConfig>()) {
            auto& sslConfig = (const SslSocketConfig &) config;
            return suil::catstr(sslConfig.bindAddr.name, ":", sslConfig.bindAddr.port);
        }
        else {
            auto& unixConfig = (const UnixSocketConfig &) config;
            return unixConfig.bindAddr.peek();
        }
    }
}