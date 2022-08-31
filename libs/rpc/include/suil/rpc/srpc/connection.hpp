//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_RPC_SRPC_CONNECTION_HPP
#define SUIL_RPC_SRPC_CONNECTION_HPP

#include <suil/rpc/srpc/common.hpp>
#include <suil/rpc/io.hpp>

namespace suil::rpc::srpc {

    class Context;

    class Connection: public RpcIO<RpcServerConfig>, public LOGGER(SUIL_RPC) {
    public:
        using LOGGER(SUIL_RPC)::log;

        Connection() = default;

        void operator()(net::Socket& sock, std::shared_ptr<Context> ctx);

    protected:
        const RpcServerConfig& getConfig() const override;

    private:
        friend class Context;

        void handleRequest(suil::HeapBoard& resp, const suil::Data& data);
        void processRequest(suil::HeapBoard& resp, suil::HeapBoard& req, int id, int method);
        template <typename T>
        suil::Data transform(const T& t) {
            suil::HeapBoard hb(Wire::maxByteSize(t)+8);
            hb << t;
            return hb.release();
        }

        std::shared_ptr<Context> context{nullptr};
    };
}
#endif //SUIL_RPC_SRPC_CONNECTION_HPP
