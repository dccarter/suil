//
// Created by Mpho Mbotho on 2021-06-29.
//

#pragma once

#include <suil/base/signal.hpp>

#include <suil/async/scope.hpp>

#include <csignal>
#include <mutex>

namespace suil {

    enum Signals : int {
        SIGNAL_INT = SIGINT,
        SIGNAL_ILL = SIGILL,
        SIGNAL_ABRT = SIGABRT,
        SIGNAL_FPE = SIGFPE,
        SIGNAL_SEGV = SIGSEGV,
        SIGNAL_TERM = SIGTERM,
        SIGNAL_HUP = SIGHUP,
        SIGNAL_QUIT = SIGQUIT,
        SIGNAL_TRAP = SIGTRAP,
        SIGNAL_KILL = SIGKILL,
        SIGNAL_BUS = SIGBUS,
        SIGNAL_SYS = SIGSYS,
        SIGNAL_PIPE = SIGPIPE,
        SIGNAL_ALRM = SIGALRM,
        SIGNAL_URG = SIGURG,
        SIGNAL_STOP = SIGSTOP,
        SIGNAL_TSTP = SIGTSTP,
        SIGNAL_CONT = SIGCONT,
        SIGNAL_CHLD = SIGCHLD,
        SIGNAL_TTIN = SIGTTIN,
        SIGNAL_TTOU = SIGTTOU,
        SIGNAL_POLL = SIGPOLL,
        SIGNAL_XCPU = SIGXCPU,
        SIGNAL_XFSZ = SIGXFSZ,
        SIGNAL_VTALRM = SIGVTALRM,
        SIGNAL_PROF = SIGPROF,
        SIGNAL_USR1 = SIGUSR1,
        SIGNAL_USR2 = SIGUSR2,
    };

    using Once = std::once_flag;

    struct ON {
        Signal<void(int, siginfo_t*, void*)>& On(Signals sig);

        AsyncVoid operator()(const std::vector<Signals>& sigs,
                             std::function<void(int, siginfo_t*, void*)> func);
        AsyncVoid operator()(const std::vector<Signals>& sigs,
                             std::function<void(int, siginfo_t*, void*)> func, Once& once);

        auto operator co_await () { return _scope.join(); }

        static ON& instance() noexcept;

        DISABLE_COPY(ON);
        DISABLE_MOVE(ON);

    private:
        ON() = default;

        using SType = Signal<void(int sig, siginfo_t *info, void *context)>;
        static void invokeSignalHandler(SType& S, int sig, siginfo_t *info, void *ctx);
        static void signalActionHandler(int sig, siginfo_t *info, void *ctx);

        std::unordered_map<Signals, std::unique_ptr<SType>> _handlers{};
        AsyncScope _scope{};
    };

    static ON& On(ON::instance());
}
