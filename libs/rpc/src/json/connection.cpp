//
// Created by Mpho Mbotho on 2020-12-03.
//

#include "suil/rpc/json/connection.hpp"
#include "suil/rpc/json/context.hpp"

namespace suil::rpc::jrpc {

    Connection::Connection()
        : RpcIO()
    {
        extensionMethods.emplace("rpc_Version", [&](const json::Object&) -> ResultCode {
            Result res = json::Object{""};
            return {0, std::move(res)};
        });
    }

    const RpcServerConfig& Connection::getConfig() const
    {
        if (_context == nullptr) {
            static RpcServerConfig defaultConfig;
            return defaultConfig;
        }
        return _context->config();
    }

    void Connection::operator()(net::Socket& sock, Context& ctx)
    {
        _context = &ctx;
        defer({
            _context = nullptr;
        });

        sock.setBuffering(false);

        try {
            Buffer ob{1024};
            do {
                ob.reset(1024, true);
                if (!Ego.receive(sock, ob))
                    break;

                auto resp = handleRequest(ob);
                if (!Ego.transmit(sock, Data{resp.data(), resp.size(), false}))
                    break;

            } while (sock.isOpen());
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            ierror("un-handled JSON RPC server error: %s", ex.what());
        }
    }

    String Connection::parseRequest(std::vector<jrpc::Request>& req, const Buffer& rxb)
    {
        try {
            json::decode(rxb, req);
            return {};
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            return String{ex.what()}.dup();
        }
    }

    std::string Connection::handleRequest(const Buffer& rxb)
    {
        std::vector<jrpc::Response> resps;
        std::vector<jrpc::Request> reqs;
        auto status = Ego.parseRequest(reqs, rxb);
        if (status) {
            /* Parsing given request failed */
            ierror("parsing request failed: " PRIs, _PRIs(status));
            RpcError err(ResultCode::ParseError, "ParseError", std::move(status));
            jrpc::Response resp;
            iod::zero(resp);
            resp.jsonrpc = String{JSON_RPC_VERSION};
            resp.error = std::move(err);
            resps.push_back(std::move(resp));
            return json::encode(resps);
        }

        for (auto& req: reqs) {
            // handle all requests
            if (req.jsonrpc != JSON_RPC_VERSION) {
                /* Only accept supported JSON RPC version */
                RpcError err{ResultCode::InvalidRequest, "InvalidRequest",
                             suil::catstr("Unsupported JSON RPC version '", req.jsonrpc, "'")};
                jrpc::Response rsp;
                iod::zero(rsp);
                rsp.jsonrpc = JSON_RPC_VERSION;
                rsp.error = std::move(err);
                resps.push_back(std::move(rsp));
                continue;
            }

            static json::Object _{nullptr};
            json::Object &obj = not(req.params.has_value())? _ : *req.params;
            auto str = json::encode(req);
            auto method = req.method.substr(0, 4);
            if (method == "rpc_") {
                resps.push_back(handleExtension(req.method, obj, *req.id));
            }
            else {
                resps.push_back(handleWithContext(*_context, req.method, obj, *req.id));
            }
        }

        return json::encode(resps);
    }

    jrpc::Response Connection::handleExtension(const String& method, const json::Object& params, int id)
    {
        jrpc::Response resp;
        iod::zero(resp);
        resp.jsonrpc = JSON_RPC_VERSION;
        resp.id = id;

        auto extMethod = extensionMethods.find(method);
        if (extMethod == extensionMethods.end()) {
            // method does not exist
            resp.error = RpcError{
                    ResultCode::MethodNotFound,
                    "InternalError",
                    suil::catstr("method '", method, "' is not an extension method")};

            return std::move(resp);
        }

        try {
            auto [code, ret] = extMethod->second(params);
            if (code) {
                resp.error = RpcError(code, "InternalError", std::get<String>(ret.Value));
            }
            else {
                resp.result = std::get<json::Object>(std::move(ret.Value));
            }
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            resp.error = RpcError(ResultCode::InternalError, "InternalError", String{ex.what()}.dup());
        }

        return std::move(resp);
    }

    jrpc::Response Connection::handleWithContext(
            jrpc::Context& ctx,
            const String& method,
            const json::Object& params, int id)
    {
        jrpc::Response resp;
        iod::zero(resp);
        resp.jsonrpc = JSON_RPC_VERSION;
        resp.id = id;

        try {
            auto str = json::encode(params);
            auto [code, ret] = ctx(method, params, id);
            if (code) {
                resp.error = RpcError(ResultCode::ApiError, "ServiceHandlerError", std::get<String>(ret.Value));
            }
            else {
                resp.result  = std::get<json::Object>(std::move(ret.Value));
            }
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            resp.error = RpcError(ResultCode::ApiError, "UnhandledApiError", String{ex.what()}.dup());
        }

        return std::move(resp);
    }
}