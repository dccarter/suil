//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_RPC_SRPC_CLIENT_HPP
#define SUIL_RPC_SRPC_CLIENT_HPP

#include <suil/rpc/wire.scc.hpp>
#include <suil/rpc/client.hpp>

namespace suil::rpc::srpc {

    define_log_tag(SRPC_CLIENT);

    class Client: public RpcClient, LOGGER(SRPC_CLIENT) {
    public:
        using LOGGER(SRPC_CLIENT)::log;
        using rpc::RpcClient::RpcClient;

        String rpcVersion() override;

        template <typename R, typename ...Params>
        R call(const String& method, Params&&... params)
        {
            suil::HeapBoard res;

            auto size = (Wire::maxByteSize(idGenerator)<<1) + paramsWireSize(params...);
            suil::HeapBoard hb{size + 16};
            hb << (idGenerator++) << getMethodId(method);
            if constexpr (sizeof...(params) > 0) {
                (hb << ... << params);
            }

            Ego.doCall(res, hb.release());
            if constexpr (std::is_same_v<suil::Data, R>) {
                return std::move(res);
            }
            else {
                return transform<R>(res);
            }
        }

        const Metadata& getMetadata();

    private:
        inline size_t paramsWireSize() { return 0; };

        template <typename Param, typename ...Params>
        size_t paramsWireSize(const Param& p, const Params&... params) {
            if constexpr (sizeof...(params) == 0) {
                return Wire::maxByteSize(p);
            }
            else {
                return sizeof Wire::maxByteSize(p) + paramsWireSize(params...);
            }
        }

        template <typename T>
        T transform(suil::HeapBoard& res)
        {
            if constexpr (!std::is_void_v<T>) {
                T tmp{};
                res.setCopyOut(true);
                res >> tmp;
                return tmp;
            }
        }

        void doCall(suil::HeapBoard& resp, const suil::Data& req);
        std::vector<suil::Data> transform(std::vector<srpc::Response>&& resps);
        int getMethodId(const String& method);

    private:
        UnorderedMap<int> methodIds;
        Metadata rpcMetadata;
    };
}

#endif //SUIL_RPC_SRPC_CLIENT_HPP
