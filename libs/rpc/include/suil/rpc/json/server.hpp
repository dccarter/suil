//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_RPC_SERVER_HPP
#define SUIL_RPC_SERVER_HPP

#include <suil/rpc/json/connection.hpp>
#include <suil/rpc/json/context.hpp>
#include <suil/rpc/server.hpp>

namespace suil::rpc::jrpc {

    using Server = RpcServer<jrpc::Connection, jrpc::Context>;
}


#endif //SUIL_RPC_SERVER_HPP
