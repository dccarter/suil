//
// Created by Mpho Mbotho on 2020-11-30.
//

#include "rpc.hpp"

#include <scc/exception.hpp>
#include <scc/program.hpp>
#include <scc/visitor.hpp>
#include <scc/includes.hpp>

namespace suil::rpc {

    void SuilRpcHppGenerator::includes(scc::Formatter fmt, scc::IncludeBag& incs)
    {
        incs.write(fmt, "suil/rpc/srpc/client.hpp");
        incs.write(fmt, "suil/rpc/srpc/context.hpp");
    }

    void SuilRpcHppGenerator::generate(scc::Formatter fmt, const scc::Type& ct)
    {
        if (!ct.is<scc::Class>()) {
            throw scc::Exception("suil/rpc/wire generator can only be used on class types");
        }
        const auto& klass = ct.as<scc::Class>();
        if (!klass.BaseClasses.empty()) {
            throw scc::Exception("suil/rpc/wire generator does not support base classes on services");
        }

        UsingNamespace(getNamespace(), fmt);
        {
            generateClient(fmt, klass);
            generateServer(fmt, klass);
        }
    }

    void SuilRpcHppGenerator::generateClient(scc::Formatter& fmt, const scc::Class& ct)
    {
        Line(fmt) << "class ";
        scc::Attribute::toString(fmt, ct.Attribs);
        fmt << ' ' << ct.Name << "SrpcClient : ";
        // implement srpc::Client
        fmt << "public suil::rpc::srpc::Client {";
        Line(fmt) << "public:";
        Line(++fmt);
        // use constructor from jrpc::Client
        Line(fmt) << "using srpc::Client::Client;";
        Line(fmt);
        bool isPublic{false};
        scc::Visitor<scc::Class>(ct).visit<scc::Node>([&](const scc::Node& tp) {
            if (tp.is<scc::Native>() and !tp.as<scc::Native>().ForCpp) {
                // dump native code as it is
                tp.toString(fmt);
            }
            else if (tp.is<scc::Modifier>()) {
                isPublic = tp.as<scc::Modifier>().Name == "public";
                tp.toString(fmt);
            }
            else if (tp.is<scc::Method>()) {
                // generate methods for the client
                generateClientMethod(fmt, tp.as<scc::Method>(), isPublic);
            }
            else if (not tp.is<scc::Field>()) {
                // all but fields
                tp.toString(fmt);
            }
        });

        Line(--fmt) << "};";
        Line(fmt);
    }

    void SuilRpcHppGenerator::generateClientMethod(
            scc::Formatter& fmt, const scc::Method& method, bool isPublic)
    {
        auto attrs = scc::Attribute::find(method.Attribs, {"rpc", "wire"});
        if (not isPublic) {
            if (not attrs.has("client")) {
                // private method are only generated on the server
                return;
            }
            method.toString(fmt);
            return;
        }

        // Only certain types of methods allowed
        if (!method.ReturnType.Kind.empty()) {
            // Return types cannot have modifiers
            throw scc::Exception(
                    "rpc/wire - method '", method.Name.Content, "' return type modifier not supported");
        }

        if (method.Const) {
            // Methods cannot be constant
            throw scc::Exception(
                    "rpc/wire - method '", method.Name.Content, "' cannot be marked as const");
        }

        // generate method
        method.toString(fmt);
    }

    void SuilRpcHppGenerator::generateServer(scc::Formatter& fmt, const scc::Class& ct)
    {
        Line(fmt) << "class ";
        scc::Attribute::toString(fmt, ct.Attribs);
        fmt << ' ' << ct.Name << "SrpcContext : ";
        // implement srpc::Context
        fmt << "public suil::rpc::srpc::Context {";
        Line(fmt) << "public:";
        Line(++fmt);
        // Create constructor
        Line(fmt) << "template <typename ...Args>";
        Line(fmt) << ct.Name << "SrpcContext(std::shared_ptr<"
                  << ct.Name << "> service, Args&&... args)";
        Line(++fmt) << ": srpc::Context::Context(std::forward<Args>(args)...)";
        Line(fmt) << ", m" << ct.Name << "{std::move(service)}";
        Line(--fmt) << "{}";

        Line(fmt);
        bool isPublic{false};
        scc::Visitor<scc::Class>(ct).visit<scc::Node>([&](const scc::Node& tp) {
            if (tp.is<scc::Native>() and !tp.as<scc::Native>().ForCpp) {
                // dump native code as it is
                tp.toString(fmt);
            }
            else if (tp.is<scc::Modifier>()) {
                isPublic = tp.as<scc::Modifier>().Name == "public";
                tp.toString(fmt);
            }
            else if (tp.is<scc::Method>()) {
                // public method are potential server methods
                generateServerMethod(fmt, tp.as<scc::Method>(), isPublic);
            }
            else {
                tp.toString(fmt);
            }

            if (tp.is<scc::Field>()) {
                fmt << ';';
            }
        });

        Line(--fmt) << "protected:";
        // add function that will be used to handle incoming messages
        Line(++fmt) << "void operator()("
                      "suil::HeapBoard& resp, suil::HeapBoard& req, int id, int method) override;";
        Line(fmt)      << "void buildMethodInfo() override;";
        --fmt;

        Line(--fmt) << "private:";
        Line(++fmt) << "std::shared_ptr<" << ct.Name << "> m" << ct.Name << "{nullptr};";
        Line(--fmt)<< "};";
        Line(fmt);
    }

    void SuilRpcHppGenerator::generateServerMethod(scc::Formatter& fmt, const scc::Method& method, bool isPublic)
    {
        auto attrs = scc::Attribute::find(method.Attribs, {"rpc", "wire"});
        if (not isPublic) {
            if (attrs.has("client") and not attrs.has("server")) {
                // private method are only generated on the server
                return;
            }
            method.toString(fmt);
            return;
        }

        Line(fmt) << "///!";
        method.toString0(fmt);
        fmt << ";";
    }

    void SuilRpcCppGenerator::generate(scc::Formatter fmt, const scc::Type& ct)
    {
        if (!ct.is<scc::Class>()) {
            throw scc::Exception("suil/rpc/wire generator can only be used on class types");
        }
        const auto& klass = ct.as<scc::Class>();
        UsingNamespace(getNamespace(), fmt);
        {
            generateClient(fmt, klass);
            generateServer(fmt, klass);
        }
    }

    void SuilRpcCppGenerator::generateClient(scc::Formatter& fmt, const scc::Class& ct)
    {
        scc::Visitor<scc::Class>(ct).visit<scc::Node>([&](const scc::Node& tp) {
            if (tp.is<scc::Native>() and tp.as<scc::Native>().ForCpp) {
                // native code meant for cpp
                tp.toString(fmt);
            }
        });

        auto rpcMethods = getRpcMethods(ct, "wire");
        for (const scc::Method& method: rpcMethods) {
            generateClientMethod(fmt, ct.Name.Content, method);
        }

    }

    void SuilRpcCppGenerator::generateClientMethod(
            scc::Formatter& fmt, const std::string& klass, const scc::Method& method)
    {
        Line(fmt);
        method.ReturnType.toString(fmt);
        fmt << ' ' << klass << "SrpcClient::" << method.Name;
        method.signature(fmt);
        fmt << " {";
        Line(++fmt);
        if (!method.ReturnType.Type.isVoid()) {
            fmt << "return ";
        }
        Line(fmt) << "Ego.call<" << method.ReturnType.Type << ">(\""
                  << method.Name << '"';
        for (auto& param: method.Params) {
            fmt << ", " << param.Name;
        }
        fmt << ");";
        Line(--fmt)<< "}";
        Line(fmt);
    }

    void SuilRpcCppGenerator::generateServer(scc::Formatter& fmt, const scc::Class& ct)
    {
        auto rpcMethods = getRpcMethods(ct, "json");
        Line(fmt) << "void " << ct.Name << "SrpcContext::buildMethodInfo() {";
        ++fmt;
        for (const scc::Method& method: rpcMethods) {
            Line(fmt) << "Ego.appendMethod(\"" << method.Name << "\");";
        }
        Line(--fmt)<< "}";
        Line(fmt);

        Line(fmt) << "void " << ct.Name
                  << "SrpcContext::operator()(suil::HeapBoard& resp, suil::HeapBoard& req, int id, int method)"
                  << " {";
        ++fmt;

        Line(fmt) << "switch(method) {";
        ++fmt;
        int index = 1;
        for (const scc::Method& method: rpcMethods) {
            Line(fmt) << "case " << index++ << ": {";
            ++fmt;
            for (auto& param: method.Params) {

                Line(fmt) << param.Type << " " << param.Name << ";";
                Line(fmt) << "req >> " << param.Name << ";";
            }

            // invoke the method
            Line(fmt);
            if (!method.ReturnType.Type.isVoid()) {
                Line(fmt) << "auto res = Ego.m" << ct.Name << "->"
                          << method.Name << '(';
                writeParameterCall(fmt, method);
                fmt << ");";
                Line(fmt) << "resp = suil::HeapBoard{payloadSize(res)};";
                Line(fmt) << "resp << id << 0 << res;";
            }
            else {
                Line(fmt) << "Ego.m" << ct.Name << "->" << method.Name << '(';
                writeParameterCall(fmt, method);
                fmt << ");";
                Line(fmt) << "resp = suil::HeapBoard{payloadSize(0)};";
                Line(fmt) << "resp << id << 0;";
            }
            Line(fmt) << "break;";
            Line(--fmt)<< "}";
        }
        Line(fmt) << "default: {";
        Line(++fmt) << R"(RpcError err{-1, "MethodNotFound", "requested method does not exist"};)";
        Line(fmt) << "resp = suil::HeapBoard(payloadSize(err));";
        Line(fmt) << "resp << id << -1 << err;";
        Line(--fmt)<< "}";   // close default case
        Line(--fmt)<< "}";   // close switch
        Line(--fmt)<< "}";   // close method
        Line(fmt);
    };
};