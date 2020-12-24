//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_CLIENT_HPP
#define SUIL_CLIENT_HPP

#include <suil/rpc/io.hpp>

namespace suil::rpc {

    define_log_tag(RPC_CLIENT);

    class RpcClient: public RpcIO<RpcClientConfig>, public LOGGER(RPC_CLIENT) {
    public:
        using LOGGER(RPC_CLIENT)::log;

        template<typename... Args>
        RpcClient(Args&& ... args)
        {
            auto opts = iod::D(std::forward<Args>(args)...);
            if (opts.has(var(tcpConfig))) {
                Ego.config.socketConfig = opts.get(var(tcpConfig), net::TcpSocketConfig{});
            } else if (opts.has(var(sslConfig))) {
                Ego.config.socketConfig = opts.get(var(sslConfig), net::SslSocketConfig{});
            } else if (opts.has(var(unixConfig))) {
                Ego.config.socketConfig = opts.get(var(unixConfig), net::UnixSocketConfig{});
            }
        }

        DISABLE_COPY(RpcClient);

        virtual bool connect();

        virtual String rpcVersion()  = 0;

        const RpcClientConfig & getConfig() const override;

    protected:
        net::Socket& sock();
        net::Socket::UPtr createAndConnect();
        int idGenerator{0};

    private:
        net::Socket::UPtr adaptor;
        RpcClientConfig config;
    };

}
#endif //SUIL_CLIENT_HPP
