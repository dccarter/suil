//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_RPC_JSON_CLIENT_HPP
#define SUIL_RPC_JSON_CLIENT_HPP

#include <suil/rpc/json/common.hpp>
#include <suil/rpc/client.hpp>

namespace suil::rpc::jrpc {

    define_log_tag(JRPC_CLIENT);

    class Client: public rpc::RpcClient, public LOGGER(JRPC_CLIENT) {
    public:
        using LOGGER(JRPC_CLIENT)::log;
        using rpc::RpcClient::RpcClient;

        String rpcVersion() override;

        template <typename... Params>
        jrpc::Result call(const String& method, Params&&... params) {
            if constexpr ((sizeof...(params) > 0)) {
                json::Object obj(json::Obj);
                obj.set(std::forward<Params>(params)...);
                return Ego.doCall(method, obj);
            }
            else {
                return Ego.doCall(method);
            }
        }

        template <typename ...Args>
        std::vector<jrpc::Result> batch(const String& method, const json::Object& params, Args&&... args) {
            std::vector<jrpc::Request> package;
            auto str = json::encode(params);
            Ego.pack(package, method, params, std::forward<Args>(args)...);
            return Ego.call(package);
        }

    private:
        template <typename ...Args>
        void pack(std::vector<jrpc::Request>& package,
                  const String& method, json::Object params, Args&&... args)
        {
            jrpc::Request req;
            iod::zero(req);
            req.method = method.peek();
            req.id = idGenerator++;
            req.jsonrpc = JSON_RPC_VERSION;
            if (!params.empty()) {
                req.params = std::move(params);
            }
            auto str = json::encode(req);
            package.push_back(std::move(req));
            if constexpr (sizeof...(args) > 0) {
                Ego.pack(package, std::forward<Args>(args)...);
            }
        }

        jrpc::Result doCall(const String& method, const json::Object& params = nullptr);
        std::vector<jrpc::Result> call(std::vector<jrpc::Request>& package);
        std::vector<jrpc::Result> transform(std::vector<jrpc::Response>&& resps);
    };
}

#endif //SUIL_RPC_JSON_CLIENT_HPP
