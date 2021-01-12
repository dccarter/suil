//
// Created by Mpho Mbotho on 2020-11-09.
//

#ifndef SUILNETWORK_SERVER_HPP
#define SUILNETWORK_SERVER_HPP

#include "suil/net/socket.hpp"
#include "suil/net/config.scc.hpp"

namespace suil::net {

    define_log_tag(SERVER);

    ServerSocket::UPtr createAdaptor(const SocketConfig& config);
    bool adaptorListen(ServerSocket& adaptor, const SocketConfig& config, int backlog);
    String getAddress(const SocketConfig& config);

    struct CustomSchedulingHandler {
    };

    template <typename Handler, class Context = void>
    class Server: LOGGER(SERVER) {
    public:
        using ContextPtr = std::shared_ptr<Context>;

        Server(ServerConfig& config, ContextPtr context = nullptr)
            : mConfig{config},
              mContext{context}
        {}

        DISABLE_MOVE(Server);
        DISABLE_COPY(Server);

        bool listen() {
            if (Adaptor == nullptr) {
                // create socket adaptor
                Adaptor = createAdaptor(mConfig.socketConfig);
            }
            auto addr = getAddress(mConfig.socketConfig);

            if (!Adaptor->isRunning()) {
                if (!adaptorListen(*Adaptor, mConfig.socketConfig, mConfig.acceptBacklog)) {
                    ierror("binding server to address '%s' failed: %s", addr(), errno_s);
                    return false;
                }

                inotice("Server listening at %s", addr());
            } else {
                idebug("server {%s} already running", addr());
            }

            return true;
        }

        virtual int start() {
            if ((Adaptor == nullptr) || !Adaptor->isRunning()) {
                // create socket adaptor
                if (!listen()) {
                    return errno;
                }
            }

            int status{EXIT_SUCCESS};

            while (!mExiting) {
                if (auto sock = Adaptor->accept(mConfig.acceptTimeout)) {
                    if constexpr (!std::is_base_of_v<CustomSchedulingHandler, Handler>) {
                        go(handle(Ego, std::move(sock)));
                    }
                    else {
                        Handler{}(std::move(sock), *mContext);
                    }
                }
                else {
                    if (errno != ETIMEDOUT) {
                        if (!mExiting) {
                            ierror("accepting next connection failed: %s", errno_s);
                            status = errno;
                        }
                    }

                    // cleanup adaptor socket
                    Adaptor->close();
                    break;
                }
            }
            mExiting = false;
            idebug("Server exiting {status=%d}", status);
            return status;
        }

        inline bool isRunning() const {
            return (Adaptor != nullptr) and Adaptor->isRunning();
        }

        virtual void stop()
        {
            idebug("stopping server...");
            mExiting = true;
            if (Adaptor) {
                Adaptor->shutdown();
            }
        }

        ~Server() {
            if (Adaptor) {
                stop();
                Adaptor->close();
                Adaptor = nullptr;
            }
        }

    private:
        static coroutine void handle(Server& Self, Socket::UPtr&& sock)
        {
            Socket::UPtr tmp = std::move(sock);
            try {
                Handler()(*tmp, *Self.mContext);
                if (tmp->isOpen()) {
                    // close socket if still open
                    tmp->close();
                }
            }
            catch (...) {
                auto ex = Exception::fromCurrent();
                ldebug(&Self, "unhandled protocol error: %s", ex.what());
            }
        }

        ServerConfig& mConfig;
        ContextPtr    mContext;
        bool          mExiting{false};
        ServerSocket::UPtr Adaptor{nullptr};
    };
}
#endif //SUILNETWORK_SERVER_HPP
