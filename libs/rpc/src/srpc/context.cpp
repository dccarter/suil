//
// Created by Mpho Mbotho on 2020-12-03.
//

#include "suil/rpc/srpc/context.hpp"
#include "suil/rpc/srpc/connection.hpp"
#include "suil/rpc/io.hpp"

namespace suil::rpc::srpc {

    RpcServerConfig& Context::config()
    {
        return serverConfig;
    }

    void Context::operator()(suil::HeapBoard& resp,
                             suil::HeapBoard& req,
                             int id,
                             int method)
    {
        RpcError err{-1, "MethodNotImplemented", "requested method not implemented"};
        resp = suil::HeapBoard(payloadSize(err));
        resp << id << -1 << err;
    }

    void Context::init()
    {
        rpcMeta.version = String{SUIL_RPC_VERSION}.dup();
        Ego.appendMethod("rpc_Version", true);
        Ego.appendMethod("rpc_enableProtoSize", true);
        buildMethodInfo();
    }

    int Context::appendMethod(String method, bool isExtension)
    {
        int id{0};
        if (isExtension) {
            for (auto& ext: rpcMeta.extensions) {
                if (ext.name == method)
                    return ext.id;
                id = ext.id;
            }
            rpcMeta.extensions.emplace_back(--id, std::move(method));
        }
        else {
            for (auto& m: rpcMeta.methods) {
                if (m.name == method)
                    return m.id;
                id = m.id;
            }
            rpcMeta.methods.emplace_back(++id, std::move(method));
        }

        return id;
    }

    void Context::handleExtension(Connection& conn,
                                          suil::HeapBoard& resp,
                                          suil::HeapBoard& req,
                                          int id,
                                          int method)
    {
        switch (method) {
            case rpcMetaRequest: {
                resp = suil::HeapBoard(payloadSize(rpcMeta));
                resp << id << 0 << rpcMeta;
                break;
            }
            case rpcVersion: {
                resp = suil::HeapBoard(payloadSize(SUIL_RPC_VERSION));
                resp << id << 0 << SUIL_RPC_VERSION;
                break;
            }
            case rpcUseProto: {
                // just read the configured value
                req >> conn.protoUseSize;
                break;
            }
            default: {
                RpcError err{-1, "UnsupportedExtensionMethod", "Extension method not found"};
                resp = suil::HeapBoard(payloadSize(err));
                resp << id << -1 << err;
            }
        }
    }
}