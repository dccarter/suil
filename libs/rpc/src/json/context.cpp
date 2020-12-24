//
// Created by Mpho Mbotho on 2020-12-03.
//

#include "suil/rpc/json/context.hpp"

namespace suil::rpc::jrpc {

    RpcServerConfig& Context::config()
    {
        return Ego.serverConfig;
    }

}