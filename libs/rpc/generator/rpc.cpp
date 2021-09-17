//
// Created by Mpho Mbotho on 2020-11-30.
//

#include "rpc.hpp"

#include <scc/exception.hpp>
#include <scc/program.hpp>
#include <scc/visitor.hpp>
#include <scc/includes.hpp>

namespace suil::rpc {

    Methods getRpcMethods(const scc::Class& ct, const std::string& gen)
    {
        auto isRpcMethod = [&](const scc::Method& m) {
            if (auto params = scc::Attribute::find(m.Attribs, {"rpc", gen})) {
                return not(params.has("client") or params.has("server"));
            }
            return true;
        };

        Methods rpcMethods;
        bool isPublic{false};
        scc::Visitor<scc::Class>(ct).visit<scc::Node>([&](const scc::Node& tp) {
            if (tp.is<scc::Method>() and isPublic and isRpcMethod(tp.as<scc::Method>())) {
                rpcMethods.push_back(tp.as<scc::Method>());
            }
            else if (tp.is<scc::Modifier>()) {
                isPublic = tp.as<scc::Modifier>().Name == "public";
            }
        });

        return std::move(rpcMethods);
    }

    void writeParameterCall(scc::Formatter& fmt, const scc::Method& method)
    {
        for (auto& param: method.Params) {
            if (&param != &method.Params.front()) {
                fmt << ", ";
            }

            if (param.Kind.empty() or param.Kind == "&&") {
                fmt << "std::move(";
                param.Name.toString(fmt);
                fmt << ')';
            }
            else if (param.Kind == "*") {
                // this really shouldn't be supported
                fmt << "&";
                param.Name.toString(fmt);
            }
            else {
                // reference used
                param.Name.toString(fmt);
            }
        }
    }
}

extern "C" {

    int LibInitialize(Context ctx) {
        scc::registerLibGenerator(
                ctx,
                "json",
                std::make_shared<suil::rpc::JsonRpcHppGenerator>(),
                std::make_shared<suil::rpc::JsonRpcCppGenerator>());
        scc::registerLibGenerator(
                ctx,
                "wire",
                std::make_shared<suil::rpc::SuilRpcHppGenerator>(),
                std::make_shared<suil::rpc::SuilRpcCppGenerator>());
        return 0;
    }

}