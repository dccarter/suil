//
// Created by Mpho Mbotho on 2021-06-29.
//

#include "suil/base/syson.hpp"

#include <suil/base/exception.hpp>
#include <suil/base/logging.hpp>

namespace suil {

    using SType = Signal<void(int sig, siginfo_t *info, void *context)>;

    static std::unordered_map<Signals, SType> s_SignalMap;

    static void invokeSignalHandler(SType& S, int sig, siginfo_t *info, void *ctx)
    {
        try {
            S(sig, info, ctx);
        }
        catch (...) {
            scritical("signal{%d} handler error: %s",
                      sig, Exception::fromCurrent().what());
        }
    }

    static void signalActionHandler(int sig, siginfo_t *info, void *ctx)
    {
        static bool s_Exiting{false};
        if (s_Exiting) {
            sdebug("received signal %d while exiting", sig);
            return;
        }

        switch (sig) {
            case SIGTERM:
            case SIGINT:
            case SIGQUIT:
                s_Exiting = true;
                break;
            default:
                break;
        }

        auto it = s_SignalMap.find(Signals(sig));
        if (it != s_SignalMap.end()) {
            invokeSignalHandler(it->second, sig, info, ctx);
        }

        exit(EXIT_SUCCESS);
    }

    SType& On(Signals sig)
    {
        if (!s_SignalMap.contains(sig)) {
            struct sigaction sa{nullptr};
            sa.sa_sigaction = &signalActionHandler                                                  ;
            sa.sa_flags = SA_SIGINFO | SA_NOCLDWAIT | SA_NOCLDSTOP;
            if (sigaction(int(sig), &sa, nullptr) == -1) {
                serror("sigaction failed: %s", errno_s);
            }
            s_SignalMap[sig] = SType{};
        }

        return s_SignalMap[sig];
    }

    static void batchOn(const std::vector<Signals>& sigs, std::function<void(int, siginfo_t*, void *)> func)
    {
        for (auto sig: sigs) {
            On(sig) += func;
        }
    }

    void On(const std::vector<Signals>& sigs, std::function<void(int, siginfo_t*, void *)> func)
    {
        batchOn(sigs, func);
    }

    void On(const std::vector<Signals>& sigs, std::function<void(int, siginfo_t*, void*)> func, Once& once)
    {
        std::call_once(once, batchOn, sigs, func);
    }
}