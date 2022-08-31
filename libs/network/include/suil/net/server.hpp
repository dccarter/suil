//
// Created by Mpho Mbotho on 2020-11-09.
//

#ifndef SUILNETWORK_SERVER_HPP
#define SUILNETWORK_SERVER_HPP

#include "suil/net/socket.hpp"
#include "suil/net/config.scc.hpp"

#include <sys/wait.h>
#include <sys/prctl.h>

namespace suil::net {

    define_log_tag(SERVER);

    ServerSocket::UPtr createAdaptor(const SocketConfig& config);
    bool adaptorListen(ServerSocket& adaptor, const SocketConfig& config, int backlog);
    String getAddress(const SocketConfig& config);
    using Postfork = std::function<void(uint32)>;

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

        template <typename... Opts>
        int start(Opts... opts) {
            struct {
                uint32 nprocs{0};
                Postfork postfork{nullptr};
            } config;

            if ((Adaptor == nullptr) || !Adaptor->isRunning()) {
                // create socket adaptor
                if (!listen()) {
                    return errno;
                }
            }
            applyConfig(config, std::forward<Opts>(opts)...);

            if (config.nprocs == 0) {
                // By default the server will use all the available CPU threads
                config.nprocs = sysconf(_SC_NPROCESSORS_ONLN);
            }

            if (config.nprocs == 1) {
                int status = accept();
                idebug("Server exiting {status=%d}", status);
                return status;
            }

            idebug("Server starting %u workers", config.nprocs);

            for (uint32 i = 0; i < config.nprocs; i++) {
                auto pid = mfork();
                if (pid > 0) {
                    idebug("Server worker-%u started {pid=%d}", i, pid);
                    continue;
                }

                prctl(PR_SET_PDEATHSIG, SIGHUP);

                if (config.postfork) {
                    config.postfork(i);
                }

                int status = accept();
                idebug("Server worker-%u exiting {status=%d}", i, status);
                return status;
            }

            // Wait for all workers to exit
            int ret = EXIT_SUCCESS;
            while (true) {
                int status = EXIT_SUCCESS;
                auto pid = wait(&status);
                if (pid <= 0)
                    break;
                ret = status == EXIT_SUCCESS? ret : status;
                idebug("Server worker exited {pid=%d, status=%d}", pid, status);
            }

            return ret;
        }

        void stop()
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
        int accept()
        {
            int status = EXIT_SUCCESS;
            while (!mExiting) {
                if (auto sock = Adaptor->accept(mConfig.acceptTimeout)) {
                    go(handle(Ego, std::move(sock)));
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
            return status;
        }

        static coroutine void handle(Server& Self, Socket::UPtr&& sock)
        {
            Socket::UPtr tmp = std::move(sock);
            try {
                Handler()(*tmp, Self.mContext);
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
