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

        generateClient(fmt, klass);
        generateServer(fmt, klass);
    }

    void SuilRpcHppGenerator::generateClient(scc::Formatter& fmt, const scc::Class& ct)
    {
        fmt() << "class ";
        scc::Attribute::toString(fmt, ct.Attribs);
        fmt(true) << ' ';
        ct.Name.toString(fmt);
        fmt(true) << "SrpcClient : ";
        // implement srpc::Client
        fmt(true) << "public suil::rpc::srpc::Client {";
        fmt() << "public:";
        fmt.push();
        // use constructor from jrpc::Client
        fmt() << "using srpc::Client::Client;";
        fmt();
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

        fmt.pop();
        fmt() << "};";
        fmt();
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
        fmt() << "class ";
        scc::Attribute::toString(fmt, ct.Attribs);
        fmt(true) << ' ';
        ct.Name.toString(fmt);
        fmt(true) << "SrpcContext : ";
        // implement srpc::Context
        fmt(true) << "public suil::rpc::srpc::Context {";
        fmt() << "public:";
        fmt.push();
        // Create constructor
        fmt() << "template <typename ...Args>";
        fmt();
        ct.Name.toString(fmt);
        fmt(true) << "SrpcContext(std::shared_ptr<";
        ct.Name.toString(fmt);
        fmt(true) << "> service, Args&&... args)";
        fmt.push() << ": srpc::Context::Context(std::forward<Args>(args)...)";
        fmt() << ", m";
        ct.Name.toString(fmt);
        fmt(true) << "{std::move(service)}";
        fmt.pop() << "{}";

        fmt();
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
                fmt(true) << ';';
            }
        });

        fmt.pop() << "protected:";
        // add function that will be used to handle incoming messages
        fmt.push() << "void operator()("
                      "suil::HeapBoard& resp, suil::HeapBoard& req, int id, int method) override;";
        fmt()      << "void buildMethodInfo() override;";
        fmt.pop();

        fmt.pop() << "private:";
        fmt.push() << "std::shared_ptr<";
        ct.Name.toString(fmt);
        fmt(true) << "> m";
        ct.Name.toString(fmt);
        fmt(true) << "{nullptr};";

        fmt.pop() << "};";
        fmt();
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

        fmt() << "///!";
        method.toString0(fmt);
        fmt(true) << ";";
    }

    void SuilRpcCppGenerator::generate(scc::Formatter fmt, const scc::Type& ct)
    {
        if (!ct.is<scc::Class>()) {
            throw scc::Exception("suil/rpc/wire generator can only be used on class types");
        }
        const auto& klass = ct.as<scc::Class>();
        generateClient(fmt, klass);
        generateServer(fmt, klass);
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
        fmt();
        method.ReturnType.toString(fmt);
        fmt(true) << ' ' << klass << "SrpcClient::";
        method.Name.toString(fmt);
        method.signature(fmt);
        fmt(true) << " {";
        fmt.push();
        if (!method.ReturnType.Type.isVoid()) {
            fmt(true) << "return ";
        }
        fmt() << "Ego.call<";
        method.ReturnType.Type.toString(fmt);
        fmt(true) << ">(\"";
        method.Name.toString(fmt);
        fmt(true) << '"';
        for (auto& param: method.Params) {
            fmt(true) << ", ";
            param.Name.toString(fmt);
        }
        fmt(true) << ");";
        fmt.pop() << "}";
        fmt();
    }

    void SuilRpcCppGenerator::generateServer(scc::Formatter& fmt, const scc::Class& ct)
    {
        auto rpcMethods = getRpcMethods(ct, "json");
        fmt() << "void ";
        ct.Name.toString(fmt);
        fmt(true) << "SrpcContext::buildMethodInfo() {";
        fmt.push(false);
        for (const scc::Method& method: rpcMethods) {
            fmt() << "Ego.appendMethod(\"";
            method.Name.toString(fmt);
            fmt(true) << "\");";
        }
        fmt.pop() << "}";
        fmt();

        fmt() << "void ";
        ct.Name.toString(fmt);
        fmt(true) << "SrpcContext::operator()(suil::HeapBoard& resp, suil::HeapBoard& req, int id, int method)";
        fmt(true) << " {";
        fmt.push(false);

        fmt() << "switch(method) {";
        fmt.push(false);
        int index = 1;
        for (const scc::Method& method: rpcMethods) {
            fmt() << "case " << index++ << ": {";
            fmt.push(false);
            for (auto& param: method.Params) {

                fmt();
                param.Type.toString(fmt);
                fmt(true) << " ";
                param.Name.toString(fmt);
                fmt(true) << ";";
                fmt() << "req >> ";
                param.Name.toString(fmt);
                fmt(true) << ";";
            }

            // invoke the method
            fmt();
            if (!method.ReturnType.Type.isVoid()) {
                fmt() << "auto res = Ego.m";
                ct.Name.toString(fmt);
                fmt(true) << "->";
                method.Name.toString(fmt);
                fmt(true) << '(';
                writeParameterCall(fmt, method);
                fmt(true) << ");";
                fmt() << "resp = suil::HeapBoard{payloadSize(res)};";
                fmt() << "resp << id << 0 << res;";
            }
            else {
                fmt() << "Ego.m";
                ct.Name.toString(fmt);
                fmt(true) << "->";
                method.Name.toString(fmt);
                fmt(true) << '(';
                writeParameterCall(fmt, method);
                fmt(true) << ");";
                fmt() << "resp = suil::HeapBoard{payloadSize(0)};";
                fmt() << "resp << id << 0;";
            }
            fmt() << "break;";
            fmt.pop() << "}";
        }
        fmt() << "default: {";
        fmt.push() << R"(RpcError err{-1, "MethodNotFound", "requested method does not exist"};)";
        fmt() << "resp = suil::HeapBoard(payloadSize(err));";
        fmt() << "resp << id << -1 << err;";
        fmt.pop() << "}";   // close default case
        fmt.pop() << "}";   // close switch
        fmt.pop() << "}";   // close method
        fmt();
    };
};