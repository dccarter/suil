//
// Created by Mpho Mbotho on 2020-11-30.
//

#ifndef SUILRPC_RPC_HPP
#define SUILRPC_RPC_HPP

#include <scc/generator.hpp>

namespace scc {
    class Class;
    class Method;
}

namespace suil::rpc {

    using Methods = std::vector<std::reference_wrapper<const scc::Method>>;
    Methods getRpcMethods(const scc::Class& ct, const std::string& gen);
    void writeParameterCall(scc::Formatter& fmt, const scc::Method& method);

    class JsonRpcHppGenerator: public scc::HppGenerator {
    public:
        void generate(scc::Formatter fmt, const scc::Type &ct) override;
        void includes(scc::Formatter fmt, scc::IncludeBag &incs) override;

    private:
        void generateClient(scc::Formatter& fmt, const scc::Class &ct);
        void generateServer(scc::Formatter& fmt, const scc::Class &ct);
        void generateServerMethod(scc::Formatter& fmt, const scc::Method &method, bool isPublic);
        void generateClientMethod(scc::Formatter& fmt, const scc::Method &method, bool isPublic);
    };

    class JsonRpcCppGenerator: public scc::CppGenerator {
    public:
        void generate(scc::Formatter fmt, const scc::Type &ct) override;

    private:
        void generateClient(scc::Formatter& fmt, const scc::Class &ct);
        void generateClientMethod(scc::Formatter& fmt, const std::string& klass, const scc::Method &method);
        void generateServer(scc::Formatter& fmt, const scc::Class &ct);
        void generateServerMethod(scc::Formatter& fmt, const std::string& klass, const scc::Method &method);
    };

    class SuilRpcHppGenerator: public scc::HppGenerator {
    public:
        void generate(scc::Formatter fmt, const scc::Type &ct) override;
        void includes(scc::Formatter fmt, scc::IncludeBag &incs) override;

    private:
        void generateClient(scc::Formatter& fmt, const scc::Class &ct);
        void generateServer(scc::Formatter& fmt, const scc::Class &ct);
        void generateServerMethod(scc::Formatter& fmt, const scc::Method &method, bool isPublic);
        void generateClientMethod(scc::Formatter& fmt, const scc::Method &method, bool isPublic);
    };

    class SuilRpcCppGenerator: public scc::CppGenerator {
    public:
        void generate(scc::Formatter fmt, const scc::Type &ct) override;

    private:
        void generateClient(scc::Formatter& fmt, const scc::Class &ct);
        void generateClientMethod(scc::Formatter& fmt, const std::string& klass, const scc::Method &method);
        void generateServer(scc::Formatter& fmt, const scc::Class &ct);
    };
}

#endif //SUILRPC_RPC_HPP
