//
// Created by Mpho Mbotho on 2020-12-03.
//

#include <suil/rpc/srpc/client.hpp>

namespace suil::rpc::srpc {

    String Client::rpcVersion()
    {
        return Ego.call<String>("rpc_Version");
    }

    int Client::getMethodId(const String& method)
    {
        constexpr int getMetadataMethodId{0};
        int id{getMetadataMethodId};

        if (!method.empty()) {
            auto& meta = Ego.getMetadata();
            if (method.substr(0, 4) == "rpc") {
                auto it = std::find_if(meta.extensions.begin(), meta.extensions.end(), [&](const auto& m) {
                    return m.name == method;
                });
                id = (it != meta.extensions.end())? it->id : id;
            }
            else {
                auto it = std::find_if(meta.methods.begin(), meta.methods.end(), [&](const auto& m) {
                    return m.name == method;
                });
                id = (it != meta.methods.end())? it->id : id;
            }

            if (id == getMetadataMethodId) {
                throw RpcApiError("SUIL RPC does not support method: '", method, "'");
            }
        }

        return id;
    }

    void Client::doCall(suil::HeapBoard& res, const suil::Data& data)
    {
        if (!Ego.transmit(sock(), data)) {
            // sending failed
            throw RpcTransportError("Sending requests to SUIL RPC server failed: ", errno_s);
        }

        Buffer rxb{};
        if (!Ego.receive(sock(), rxb)) {
            // receiving response failed
            throw RpcTransportError("Receiving SUIL RPC response from server failed: ", errno_s);
        }

        try {
            auto size = rxb.size();
            res = suil::HeapBoard(reinterpret_cast<uint8_t *>(rxb.release()), size, true);
            int id{0}, code{0};
            res >> id;
            res >> code;
            if (code != 0) {
                RpcError err;
                // transform the error
                res >> err;
                throw RpcApiError(err.message, "-", err.code, " ", err.data);
            }
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            throw RpcInternalError("Failed to decode SUIL RPC response: ", ex.what());
        }
    }

    const Metadata& Client::getMetadata()
    {
        if (Ego.rpcMetadata.version.empty()) {
            Ego.rpcMetadata = Ego.call<Metadata>("");
        }

        return Ego.rpcMetadata;
    }
}