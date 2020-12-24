//
// Created by Mpho Mbotho on 2020-12-03.
//

#include "suil/rpc/json/client.hpp"

namespace suil::rpc::jrpc {

    String Client::rpcVersion()
    {
        auto res = Ego.call("rpc_Version");
        if (res.has<String>()) {
            auto& err = std::get<String>(res.Value);
            ierror("service returned error: " PRIs, _PRIs(err));
            return {};
        }
        else {
            auto& obj = std::get<json::Object>(res.Value);
            String ver{};
            obj.copyOut(ver, obj);
            return ver;
        }
    }

    jrpc::Result Client::doCall(const String& method, const json::Object& params)
    {
        auto resps = Ego.batch(method, params);
        auto resp = std::move(resps.back());
        resps.pop_back();

        return std::move(resp);
    }

    std::vector<jrpc::Result> Client::call(std::vector<jrpc::Request>& package)
    {
        std::vector<jrpc::Response> resps;
        auto raw = json::encode(package);
        if (!Ego.transmit(sock(), Data{raw.data(), raw.size(), false})) {
            // sending failed
            throw RpcTransportError("Sending requests to JSON RPC server failed: ", errno_s);
        }

        Buffer rxb{};
        if (!Ego.receive(sock(), rxb)) {
            // receiving response failed
            throw RpcTransportError("Receiving JSON RPC response from server failed: ", errno_s);
        }

        try {
            json::decode(rxb, resps);
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            throw RpcInternalError("Failed to decode JSON RPC response: ", ex.what());
        }

        return Ego.transform(std::move(resps));
    }

    std::vector<jrpc::Result> Client::transform(std::vector<jrpc::Response>&& resps)
    {
        std::vector<jrpc::Result> res;
        while (!resps.empty()) {
            auto resp = std::move(resps.back());
            resps.pop_back();
            if (resp.error and resp.result) {
                // response can either be an error or a result
                throw RpcInternalError(
                        "Response {id=", *resp.id, "} is valid because it payload has a result and an error");
            }

            if (resp.error) {
                // transform the error
                auto& err = *resp.error;
                if (err.code >= -32099 and err.code <= ResultCode::ApiError) {
                    // this an API error
                    jrpc::Result result{suil::catstr("ApiError-", err.code, " ", err.data)};
                    res.push_back(std::move(result));
                }
                else {
                    /* System error */
                    throw RpcInternalError(
                            "Internal server error ",err.message, "-", err.code, " ", err.data);
                }
            }
            else {
                // push back result
                jrpc::Result jres{std::move(*resp.result)};
                res.emplace_back(std::move(jres));
            }
        }

        return std::move(res);
    }

}