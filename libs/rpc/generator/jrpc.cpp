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

        UsingNamespace(getNamespace(), fmt);
        {
            generateClient(fmt, klass);

            Line(fmt) << "class " << ct.Name<< ";";

            generateServer(fmt, klass);
        }
    }

    void JsonRpcHppGenerator::generateClient(scc::Formatter& fmt, const scc::Class& ct)
    {
        Line(fmt) << "class ";
        scc::Attribute::toString(fmt, ct.Attribs);
        fmt << ' ' << ct.Name << "JrpcClient : ";
        // implement Jrpc::Client
        fmt << "public suil::rpc::jrpc::Client {";
        Line(fmt) << "public:";
        Line(++fmt);
        // use constructor from jrpc::Client
        Line(fmt) << "using jrpc::Client::Client;";
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

        Line(--fmt)<< "};";
        Line(fmt);
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
        Line(fmt) << "class ";
        scc::Attribute::toString(fmt, ct.Attribs);
        fmt << ' ' << ct.Name << "JrpcContext : ";
        // implement jrpc::Context
        fmt << "public suil::rpc::jrpc::Context {";
        Line(fmt) << "public:";
        Line(++fmt);
        // Create constructor
        Line(fmt) << "template <typename ...Args>" << ct.Name
                  << "JrpcContext(std::shared_ptr<"
                  << ct.Name << "> service, Args&&... args)";
        Line(++fmt) << ": jrpc::Context::Context(std::forward<Args>(args)...)";
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
        Line(++fmt) << "suil::rpc::jrpc::ResultCode operator()("
                      "const suil::String& method, const suil::json::Object& params, int id) override;";
        --fmt;

        Line(--fmt) << "private:";
        Line(++fmt) << "std::shared_ptr<" << ct.Name << "> m" << ct.Name << "{nullptr};";
        Line(--fmt) << "};";
        Line(fmt);
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

        Line(fmt) << "///!";
        method.toString0(fmt);
        fmt << ";";
    }

    void JsonRpcCppGenerator::generate(scc::Formatter fmt, const scc::Type& ct)
    {
        if (!ct.is<scc::Class>()) {
            throw scc::Exception("suil/rpc/json generator can only be used on class types");
        }
        const auto& klass = ct.as<scc::Class>();
        UsingNamespace(getNamespace(), fmt);
        {
            generateClient(fmt, klass);
            generateServer(fmt, klass);
        }
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
        Line(fmt);
        method.ReturnType.toString(fmt);
        fmt << ' ' << klass << "JrpcClient::" << method.Name;
        method.signature(fmt);
        fmt << " {";
        Line(++fmt) << R"(auto res = Ego.call(")"
                    << method.Name << '"';
        for (auto& param: method.Params) {
            fmt << ", \"";
            param.Name.toString(fmt);
            fmt << "\", ";
            param.Name.toString(fmt);
        }
        fmt << ");";
        Line(fmt) << "if (res.has<suil::String>()) {";
        Line(++fmt) << "auto& err = std::get<suil::String>(res.Value);";
        Line(fmt)      << "throw suil::rpc::RpcApiError(err);";
        Line(--fmt)<< "}";
        if (!method.ReturnType.Type.isVoid()) {
            Line(fmt) << "auto& obj = std::get<suil::json::Object>(res.Value);"
                      << method.ReturnType.Type << " tmp;";
            Line(fmt) << "obj.copyOut(tmp, obj);";
            Line(fmt) << "return std::move(tmp);";
        }
        Line(--fmt)<< "}";
        Line(fmt);
    }

    void JsonRpcCppGenerator::generateServer(scc::Formatter& fmt, const scc::Class& ct)
    {
        Line(fmt) << "suil::rpc::jrpc::ResultCode " << ct.Name
                  << "JrpcContext::operator()("
                  << "const suil::String& method, const suil::json::Object& params, int id) {";
        Line(++fmt) << "static suil::UnorderedMap<int> scMappings = {";
        ++fmt;

        auto rpcMethods = getRpcMethods(ct, "json");
        int index{0};
        for (const scc::Method& method: rpcMethods) {
            if (index != 0) {
                fmt << ",";
            }
            Line(fmt) << R"({")" << method.Name << R"(", )" << index++ << "}";
        }
        Line(--fmt)<< "};";
        Line(fmt);
        Line(fmt) << "if (m" << ct.Name << " == nullptr) {";
        Line(++fmt) << "return {suil::rpc::jrpc::ResultCode::InternalError,"
                      "suil::String{\"Service currently unavailbale\"}};";
        Line(--fmt)<< "}";
        Line(fmt);

        Line(fmt) << "auto it = scMappings.find(method);";
        Line(fmt) << "if (it == scMappings.end()) {";
        Line(++fmt) << "return {suil::rpc::jrpc::ResultCode::MethodNotFound,"
                               "suil::String{\"request method not implemented\"}};";
        Line(--fmt)<< "}";
        Line(fmt);
        Line(fmt) << "switch(it->second) {";
        ++fmt;
        index = 0;
        for (const scc::Method& method: rpcMethods) {
            Line(fmt) << "case " << index++ << ": {";
            ++fmt;
            for (auto& param: method.Params) {
                Line(fmt) << "auto " << param.Name << " = ("
                          << param.Type << ") params[\""
                          << param.Name
                          << "\"];";
            }
            // invoke the method
            Line(fmt);
            if (!method.ReturnType.Type.isVoid()) {
                Line(fmt) << "json::Object obj{Ego.m" << ct.Name
                          << "->" << method.Name << '(';
                writeParameterCall(fmt, method);
                fmt << ")};";
                Line(fmt) << "return {0, jrpc::Result{std::move(obj)}};";
            }
            else {
                Line(fmt) << "Ego.m" << ct.Name
                          << "->"
                          << method.Name << '(';
                writeParameterCall(fmt, method);
                fmt << ");";
                Line(fmt) << "return {0, jrpc::Result{suil::json::Object(nullptr)}};";
            }
            Line(--fmt)<< "}";
        }
        Line(fmt) << "default: {";
        Line(++fmt) << "return {suil::rpc::jrpc::ResultCode::MethodNotFound, "
                        "suil::String(\"requested method does not exist\")};";
        Line(--fmt)<< "}";   // close default case
        Line(--fmt)<< "}";   // close switch
        Line(--fmt)<< "}";   // close method
        Line(fmt);
    };
}