//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_RPC_JSON_COMMON_HPP
#define SUIL_RPC_JSON_COMMON_HPP

#include <suil/rpc/json.scc.hpp>

#ifndef JSON_RPC_VERSION
#define JSON_RPC_VERSION "2.0"
#endif

namespace suil::rpc::jrpc {

    define_log_tag(JSON_RPC);

    struct ResultCode {
        enum: int {
            ApiError         = -32000,
            ParseError       = -32700,
            InvalidRequest   = -32600,
            MethodNotFound   = -32601,
            InvalidParams    = -32002,
            InternalError    = -32603,
        };

        ResultCode() = default;
        ResultCode(int code, Result res)
            : code{code},
              result{std::move(res)}
        {}

        int code;
        Result result;
    };

}
#endif //SUIL_RPC_JSON_COMMON_HPP
