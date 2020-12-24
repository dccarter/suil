//
// Created by Mpho Mbotho on 2020-11-29.
//

#ifndef SUILRPC_IO_HPP
#define SUILRPC_IO_HPP

#include "suil/rpc/common.scc.hpp"

#include <suil/net/socket.hpp>
#include <suil/net/server.hpp>

#include <list>

namespace suil::rpc {

    template <typename Connection, typename Context>
    class RpcServer: public net::Server<Connection, Context> {
    public:
        template<typename... Args>
        explicit RpcServer(std::shared_ptr<Context> ctx, Args&&... args)
                : net::Server<Connection, Context>(serverConfig, ctx)
        {
            auto opts = iod::D(std::forward<Args>(args)...);
            if (opts.has(var(tcpConfig))) {
                Ego.serverConfig.socketConfig = opts.get(var(tcpConfig), net::TcpSocketConfig{});
            }
            else if (opts.has(var(sslConfig))) {
                Ego.serverConfig.socketConfig = opts.get(var(sslConfig), net::SslSocketConfig{});
            }
            else if (opts.has(var(unixConfig))) {
                Ego.serverConfig.socketConfig = opts.get(var(unixConfig), net::UnixSocketConfig{});
            }
        }

    private:
        net::ServerConfig serverConfig;
    };
}

#endif //SUILRPC_IO_HPP
