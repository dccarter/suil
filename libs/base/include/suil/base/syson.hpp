//
// Created by Mpho Mbotho on 2021-06-29.
//

#pragma once

#include <suil/base/signal.hpp>

#include <signal.h>
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
    Signal<void(int, siginfo_t*, void*)>& On(Signals sig);
    void On(const std::vector<Signals>& sigs, std::function<void(int, siginfo_t*, void*)> func);
    void On(const std::vector<Signals>& sigs, std::function<void(int, siginfo_t*, void*)> func, Once& once);
}
