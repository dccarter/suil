//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_RPC_CONTEXT_HPP
#define SUIL_RPC_CONTEXT_HPP

#include <suil/rpc/json/common.hpp>

namespace suil::rpc::jrpc {

    class Context : public LOGGER(JSON_RPC) {
        public:
        sptr(Context);
        DISABLE_COPY(Context);
        DISABLE_MOVE(Context);

        template <typename ...Args>
        Context(Args&&... args)
        {
            suil::applyConfig(serverConfig, std::forward<Args>(args)...);
        }

        virtual ResultCode operator()(const String& method, const json::Object& params, int id) {
            return {ResultCode::MethodNotFound, String{"method not implemented"}};
        }

        RpcServerConfig& config();

        protected:
        RpcServerConfig serverConfig;
    };
}

#endif //SUIL_RPC_CONTEXT_HPP
