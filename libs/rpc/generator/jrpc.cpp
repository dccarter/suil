//
// Created by Mpho Mbotho on 2020-11-30.
//

#include "rpc.hpp"

#include <scc/exception.hpp>
#include <scc/program.hpp>
#include <scc/visitor.hpp>
#include <scc/includes.hpp>

namespace suil::rpc {

    void JsonRpcHppGenerator::includes(scc::Formatter fmt, scc::IncludeBag& incs)
    {
        incs.write(fmt, "suil/rpc/json/client.hpp");
        incs.write(fmt, "suil/rpc/json/context.hpp");
    }

    void JsonRpcHppGenerator::generate(scc::Formatter fmt, const scc::Type& ct)
    {
        if (!ct.is<scc::Class>()) {
            throw scc::Exception("suil/rpc/json generator can only be used on class types");
        }
        const auto& klass = ct.as<scc::Class>();
        if (!klass.BaseClasses.empty()) {
            throw scc::Exception("suil/rpc/json generator does not support base classes on services");
        }

        generateClient(fmt, klass);

        fmt() << "class ";
        ct.Name.toString(fmt);
        fmt(true) << ";";

        generateServer(fmt, klass);
    }

    void JsonRpcHppGenerator::generateClient(scc::Formatter& fmt, const scc::Class& ct)
    {
        fmt() << "class ";
        scc::Attribute::toString(fmt, ct.Attribs);
        fmt(true) << ' ';
        ct.Name.toString(fmt);
        fmt(true) << "JrpcClient : ";
        // implement Jrpc::Client
        fmt(true) << "public suil::rpc::jrpc::Client {";
        fmt() << "public:";
        fmt.push();
        // use constructor from jrpc::Client
        fmt() << "using jrpc::Client::Client;";
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

        fmt.pop() << "};";
        fmt();
    }

    void JsonRpcHppGenerator::generateClientMethod(
            scc::Formatter& fmt, const scc::Method& method, bool isPublic)
    {
        auto attrs = scc::Attribute::find(method.Attribs, {"rpc", "json"});
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
                    "rpc/json - method '", method.Name.Content, "' return type modifier not supported");
        }

        if (method.Const) {
            // Methods cannot be constant
            throw scc::Exception(
                    "rpc/json - method '", method.Name.Content, "' cannot be marked as const");
        }

        // generate method
        method.toString(fmt);
    }

    void JsonRpcHppGenerator::generateServer(scc::Formatter& fmt, const scc::Class& ct)
    {
        fmt() << "class ";
        scc::Attribute::toString(fmt, ct.Attribs);
        fmt(true) << ' ';
        ct.Name.toString(fmt);
        fmt(true) << "JrpcContext : ";
        // implement jrpc::Context
        fmt(true) << "public suil::rpc::jrpc::Context {";
        fmt() << "public:";
        fmt.push();
        // Create constructor
        fmt() << "template <typename ...Args>";
        ct.Name.toString(fmt);
        fmt(true) << "JrpcContext(std::shared_ptr<";
        ct.Name.toString(fmt);
        fmt(true) << "> service, Args&&... args)";
        fmt.push() << ": jrpc::Context::Context(std::forward<Args>(args)...)";
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
        fmt.push() << "suil::rpc::jrpc::ResultCode operator()("
                      "const suil::String& method, const suil::json::Object& params, int id) override;";
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

    void JsonRpcHppGenerator::generateServerMethod(scc::Formatter& fmt, const scc::Method& method, bool isPublic)
    {
        auto attrs = scc::Attribute::find(method.Attribs, {"rpc", "json"});
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

    void JsonRpcCppGenerator::generate(scc::Formatter fmt, const scc::Type& ct)
    {
        if (!ct.is<scc::Class>()) {
            throw scc::Exception("suil/rpc/json generator can only be used on class types");
        }
        const auto& klass = ct.as<scc::Class>();
        generateClient(fmt, klass);
        generateServer(fmt, klass);
    }

    void JsonRpcCppGenerator::generateClient(scc::Formatter& fmt, const scc::Class& ct)
    {
        scc::Visitor<scc::Class>(ct).visit<scc::Node>([&](const scc::Node& tp) {
            if (tp.is<scc::Native>() and tp.as<scc::Native>().ForCpp) {
                // native code meant for cpp
                tp.toString(fmt);
            }
        });

        auto rpcMethods = getRpcMethods(ct, "json");
        for (const scc::Method& method: rpcMethods) {
            generateClientMethod(fmt, ct.Name.Content, method);
        }

    }

    void JsonRpcCppGenerator::generateClientMethod(
            scc::Formatter& fmt, const std::string& klass, const scc::Method& method)
    {
        fmt();
        method.ReturnType.toString(fmt);
        fmt(true) << ' ' << klass << "JrpcClient::";
        method.Name.toString(fmt);
        method.signature(fmt);
        fmt(true) << " {";
        fmt.push() << R"(auto res = Ego.call(")";
        method.Name.toString(fmt);
        fmt(true) << '"';
        for (auto& param: method.Params) {
            fmt(true) << ", \"";
            param.Name.toString(fmt);
            fmt(true) << "\", ";
            param.Name.toString(fmt);
        }
        fmt(true) << ");";
        fmt() << "if (res.has<suil::String>()) {";
        fmt.push() << "auto& err = std::get<suil::String>(res.Value);";
        fmt()      << "throw suil::rpc::RpcApiError(err);";
        fmt.pop() << "}";
        if (!method.ReturnType.Type.isVoid()) {
            fmt() << "auto& obj = std::get<suil::json::Object>(res.Value);";
            method.ReturnType.toString(fmt);
            fmt(true) << " tmp;";
            fmt() << "obj.copyOut(tmp, obj);";
            fmt() << "return std::move(tmp);";
        }
        fmt.pop() << "}";
        fmt();
    }

    void JsonRpcCppGenerator::generateServer(scc::Formatter& fmt, const scc::Class& ct)
    {
        fmt() << "suil::rpc::jrpc::ResultCode ";
        ct.Name.toString(fmt);
        fmt(true) << "JrpcContext::operator()(";
        fmt(true) << "const suil::String& method, const suil::json::Object& params, int id) {";
        fmt.push() << "static suil::UnorderedMap<int> scMappings = {";
        fmt.push(false);

        auto rpcMethods = getRpcMethods(ct, "json");
        int index{0};
        for (const scc::Method& method: rpcMethods) {
            if (index != 0) {
                fmt(true) << ",";
            }
            fmt() << R"({")";
            method.Name.toString(fmt);
            fmt(true) << R"(", )" << index++ << "}";
        }
        fmt.pop() << "};";
        fmt();
        fmt() << "if (m";
        ct.Name.toString(fmt);
        fmt(true) << " == nullptr) {";
        fmt.push() << "return {suil::rpc::jrpc::ResultCode::InternalError,"
                      "suil::String{\"Service currently unavailbale\"}};";
        fmt.pop() << "}";
        fmt();

        fmt() << "auto it = scMappings.find(method);";
        fmt() << "if (it == scMappings.end()) {";
        fmt.push() << "return {suil::rpc::jrpc::ResultCode::MethodNotFound,"
                               "suil::String{\"request method not implemented\"}};";
        fmt.pop() << "}";
        fmt();
        fmt() << "switch(it->second) {";
        fmt.push(false);
        index = 0;
        for (const scc::Method& method: rpcMethods) {
            fmt() << "case " << index++ << ": {";
            fmt.push(false);
            for (auto& param: method.Params) {

                fmt() << "auto ";
                param.Name.toString(fmt);
                fmt(true) << " = (";
                param.Type.toString(fmt);
                fmt(true) << ") params[\"";
                param.Name.toString(fmt);
                fmt(true) << "\"];";
            }
            // invoke the method
            fmt();
            if (!method.ReturnType.Type.isVoid()) {
                fmt() << "json::Object obj{Ego.m";
                ct.Name.toString(fmt);
                fmt(true) << "->";
                method.Name.toString(fmt);
                fmt(true) << '(';
                writeParameterCall(fmt, method);
                fmt(true) << ")};";
                fmt() << "return {0, jrpc::Result{std::move(obj)}};";
            }
            else {
                fmt() << "Ego.m";
                ct.Name.toString(fmt);
                fmt(true) << "->";
                method.Name.toString(fmt);
                fmt(true) << '(';
                writeParameterCall(fmt, method);
                fmt(true) << ");";
                fmt() << "return {0, jrpc::Result{suil::json::Object(nullptr)}};";
            }
            fmt.pop() << "}";
        }
        fmt() << "default: {";
        fmt.push() << "return {suil::rpc::jrpc::ResultCode::MethodNotFound, "
                        "suil::String(\"requested method does not exist\")};";
        fmt.pop() << "}";   // close default case
        fmt.pop() << "}";   // close switch
        fmt.pop() << "}";   // close method
        fmt();
    };
}