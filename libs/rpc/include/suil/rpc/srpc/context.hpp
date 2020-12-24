//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_RPC_SRPC_CONTEXT_HPP
#define SUIL_RPC_SRPC_CONTEXT_HPP

#include <suil/rpc/srpc/common.hpp>

namespace suil::rpc::srpc {

    class Connection;

    class Context : public LOGGER(SUIL_RPC) {
    public:
    sptr(Context);
        DISABLE_COPY(Context);
        DISABLE_MOVE(Context);

        template <typename ...Args>
        Context(Args&&... args)
        {
            suil::applyConfig(serverConfig, std::forward<Args>(args)...);
        }

        virtual void operator()(suil::HeapBoard& resp,
                                        suil::HeapBoard& req,
                                        int id,
                                        int method);

        RpcServerConfig& config();

        template <typename T>
        srpc::Result transform(const T& t)
        {
            try {
                suil::HeapBoard hb(Wire::maxByteSize(t) + 8);
                hb << t;
                return hb.release();
            }
            catch (...) {
                auto ex = Exception::fromCurrent();
                return RpcError(-1, "EncodingError", String{ex.what()}.dup());
            }
        }

        void handleExtension(Connection& conn,
                             suil::HeapBoard& resp,
                             suil::HeapBoard& req,
                             int id,
                             int method);

        void init();

        template <typename T>
        static inline size_t payloadSize(const T& t) {
            static size_t hz{(Wire::maxByteSize(int(0))<<1)+16};
            return hz + Wire::maxByteSize(t);
        }

    protected:
        int appendMethod(String method, bool isExtension = false);
        virtual void buildMethodInfo() {}
    private:
        static constexpr int rpcMetaRequest{0};
        static constexpr int rpcVersion{-1};
        static constexpr int rpcUseProto{-2};

        RpcServerConfig serverConfig;
        srpc::Metadata rpcMeta;
    };
}

#endif //SUIL_RPC_SRPC_CONTEXT_HPP
