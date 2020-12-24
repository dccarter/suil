//
// Created by Mpho Mbotho on 2020-12-03.
//

#include <suil/rpc/client.hpp>

#include <suil/net/ssl.hpp>
#include <suil/net/tcp.hpp>
#include <suil/net/unix.hpp>

namespace suil::rpc {

    net::Socket::UPtr RpcClient::createAndConnect()
    {
        if (getConfig().socketConfig.has<net::TcpSocketConfig>()) {
            auto& tcpConfig = (const net::TcpSocketConfig &) getConfig().socketConfig;

            auto addr = ipremote(
                    tcpConfig.bindAddr.name(),
                    tcpConfig.bindAddr.port,
                    0,
                    getConfig().connectTimeout);
            if (errno != 0) {
                ierror("Resolving RPC TCP server address " PRIs ":%d failed: %s",
                       _PRIs(tcpConfig.bindAddr.name), tcpConfig.bindAddr.port, errno_s);
                return nullptr;
            }

            auto tcpSock = std::make_unique<net::TcpSock>();
            if (!tcpSock->connect(addr, getConfig().connectTimeout)) {
                // connecting to server failed
                ierror("RPC client connection to TCP server address %s failed: %s",
                       net::Socket::ipstr(addr), errno_s);
                return nullptr;
            }
            idebug("RPC client connected to server %s", net::Socket::ipstr(addr));
            return tcpSock;
        }
        else if (getConfig().socketConfig.has<net::SslSocketConfig>()) {
            auto& sslConfig = (const net::SslSocketConfig &) getConfig().socketConfig;

            auto addr = ipremote(
                    sslConfig.bindAddr.name(),
                    sslConfig.bindAddr.port,
                    0,
                    getConfig().connectTimeout);
            if (errno != 0) {
                ierror("Resolving RPC SSL server address " PRIs ":%d failed: %s",
                       _PRIs(sslConfig.bindAddr.name), sslConfig.bindAddr.port, errno_s);
                return nullptr;
            }

            auto sslSock = std::make_unique<net::SslSock>();
            if (!sslSock->connect(addr, getConfig().connectTimeout)) {
                // connecting to server failed
                ierror("RPC client connection to SSL server address %s failed: %s",
                       net::Socket::ipstr(addr), errno_s);
                return nullptr;
            }
            idebug("RPC client connected to server %s", net::Socket::ipstr(addr));
            return sslSock;
        }
        else if (getConfig().socketConfig.has<net::UnixSocketConfig>()) {
            auto& unixConfig = (const net::UnixSocketConfig &) getConfig().socketConfig;

            auto unixSock = std::make_unique<net::UnixSocket>();
            if (!unixSock->connect(unixConfig.bindAddr, getConfig().connectTimeout)) {
                // connecting to server failed
                ierror("RPC client connection to Unix server address " PRIs " failed: %s",
                       _PRIs(unixConfig.bindAddr), errno_s);
                return nullptr;
            }
            idebug("RPC client connected to Unix server " PRIs, _PRIs(unixConfig.bindAddr));
            return unixSock;
        }

        return nullptr;
    }

    bool RpcClient::connect()
    {
        if (adaptor != nullptr) {
            idebug("JsonRpcClient already connected");
            return true;
        }

        adaptor = createAndConnect();
        if (adaptor == nullptr) {
            // should have already been logged
            return false;
        }
        adaptor->setBuffering(false);
        return true;
    }

    net::Socket& RpcClient::sock()
    {
        if (adaptor == nullptr) {
            throw RpcTransportError("Client is current not connected to server");
        }

        return *adaptor;
    }

    const RpcClientConfig& RpcClient::getConfig() const
    {
        return config;
    }
}