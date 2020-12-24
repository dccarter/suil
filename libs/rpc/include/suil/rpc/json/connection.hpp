//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_RPC_JSON_CONNECTION_HPP
#define SUIL_RPC_JSON_CONNECTION_HPP

#include <suil/rpc/json/common.hpp>
#include <suil/rpc/io.hpp>

namespace suil::rpc::jrpc {

    class Context;

    class Connection: public RpcIO<RpcServerConfig>, public LOGGER(JSON_RPC) {
    public:
        using LOGGER(JSON_RPC)::log;
        Connection();

        void operator()(net::Socket& sock, std::shared_ptr<Context> ctx);

    protected:
        const RpcServerConfig& getConfig() const override;

    private:
        String parseRequest(std::vector<rpc::jrpc::Request>& req, const Buffer& rxb);
        std::string handleRequest(const Buffer& req);
        rpc::jrpc::Response handleExtension(const String& method, const json::Object& req, int id = 0);
        rpc::jrpc::Response handleWithContext(Context& ctx, const String& method, const json::Object& req, int id = 0);

    private:
        using ExtensionMethod = std::function<rpc::jrpc::ResultCode(const json::Object&)>;
        std::shared_ptr<Context> context{nullptr};
        UnorderedMap<ExtensionMethod> extensionMethods;
    };
}
#endif //SUIL_RPC_JSON_CONNECTION_HPP
