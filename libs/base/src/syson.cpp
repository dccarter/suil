//
// Created by Mpho Mbotho on 2021-06-29.
//

#include "suil/base/syson.hpp"
#include "suil/base/logging.hpp"

#include <suil/utils/exception.hpp>
#include <suil/async/scope.hpp>


namespace suil {

    using SType = Signal<void(int sig, siginfo_t *info, void *context)>;

    static std::unordered_map<Signals, std::unique_ptr<SType>> s_SignalMap;

    void ON::invokeSignalHandler(SType& S, int sig, siginfo_t *info, void *ctx)
    {
        try {
            ON::instance()._scope.spawn(S(sig, info, ctx));
        }
        catch (...) {
            scritical("signal{%d} handler error: %s",
                      sig, Exception::fromCurrent().what());
        }
    }

    void ON::signalActionHandler(int sig, siginfo_t *info, void *ctx)
    {
        static std::atomic_bool s_Exiting{false};
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
            invokeSignalHandler(*it->second, sig, info, ctx);
        }

        exit(EXIT_SUCCESS);
    }

    SType& ON::On(Signals sig)
    {
        if (!s_SignalMap.contains(sig)) {
            struct sigaction sa{nullptr};
            sa.sa_sigaction = &signalActionHandler                                                  ;
            sa.sa_flags = SA_SIGINFO | SA_NOCLDWAIT | SA_NOCLDSTOP;
            if (sigaction(int(sig), &sa, nullptr) == -1) {
                serror("sigaction failed: %s", errno_s);
            }
            s_SignalMap[sig] = std::make_unique<SType>();
        }

        return *s_SignalMap[sig];
    }

    static AsyncVoid batchOn(const std::vector<Signals>& sigs, std::function<void(int, siginfo_t*, void *)> func)
    {
        for (auto sig: sigs) {
            co_await (ON::instance().On(sig) += func);
        }
    }

    AsyncVoid ON::operator()(const std::vector<Signals>& sigs, std::function<void(int, siginfo_t*, void *)> func)
    {
        return batchOn(sigs, std::move(func));
    }

    AsyncVoid ON::operator()(const std::vector<Signals>& sigs,
                            std::function<void(int, siginfo_t*, void *)> func,
                            Once& once)
    {
        return batchOn(sigs, std::move(func));
    }

    ON& ON::instance() noexcept
    {
        static ON s_ON;
        return s_ON;
    }
}