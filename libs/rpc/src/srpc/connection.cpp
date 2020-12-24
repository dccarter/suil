//
// Created by Mpho Mbotho on 2020-12-03.
//

#include "suil/rpc/srpc/connection.hpp"
#include "suil/rpc/srpc/context.hpp"

namespace suil::rpc::srpc {

    const RpcServerConfig& Connection::getConfig() const
    {
        if (context != nullptr) {
            return context->config();
        }
        static  RpcServerConfig defaultServerConfig;
        return defaultServerConfig;
    }

    void Connection::operator()(net::Socket& sock, std::shared_ptr<Context> ctx)
    {
        Ego.context = std::move(ctx);
        sock.setBuffering(false);

        try {
            Buffer ob{1024};
            do {
                ob.reset(1024, true);
                if (!Ego.receive(sock, ob))
                    break;

                suil::HeapBoard resp;
                handleRequest(resp, ob.cdata());
                if (!Ego.transmit(sock, resp.release()))
                    break;

            } while (sock.isOpen());
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            ierror("un-handled SUIL RPC server {client=%s} error: %s", sock.id(), ex.what());
        }
    }

    void Connection::handleRequest(suil::HeapBoard& resp, const suil::Data& data)
    {
        int id{-1};
        try {
            suil::HeapBoard hb(data);

            int method{0};
            hb >> id >> method;
            processRequest(resp, hb, id, method);
        }
        catch (...) {
            // unhandled error
            auto ex = Exception::fromCurrent();
            RpcError err{-1, "DecodingError", ex.what()};
            resp = suil::HeapBoard(Context::payloadSize(err));
            resp << id << -1 << err;
        }
    }

    void Connection::processRequest(suil::HeapBoard& resp, suil::HeapBoard& req, int id, int method)
    {
        try {
            if (method <= 0) {
                // extension request
                context->handleExtension(Ego, resp, req, id, method);
            }
            else {
                (*context)(resp, req, id, method);
            }
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            RpcError err{-1, "ApiError", ex.what()};
            resp = suil::HeapBoard(Context::payloadSize(err));
            resp << id << -1 << err;
        }
    }
}