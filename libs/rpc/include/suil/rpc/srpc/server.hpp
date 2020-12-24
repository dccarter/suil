//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_RPC_SRPC_SERVER_HPP
#define SUIL_RPC_SRPC_SERVER_HPP

#include <suil/rpc/srpc/context.hpp>
#include <suil/rpc/srpc/connection.hpp>
#include <suil/rpc/server.hpp>

namespace suil::rpc::srpc {

    using Server = RpcServer<srpc::Connection, srpc::Context>;
}
#endif //SUIL_RPC_SRPC_SERVER_HPP
