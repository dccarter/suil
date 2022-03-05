/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-07
 */

#pragma once

#include <suil/async/delay.hpp>
#include <suil/async/fdwait.hpp>

#include <span>
#include <set>
#include <unordered_map>
#include <variant>

namespace suil {

    struct AsyncResp {
        void *ptr{nullptr};
        FDW res{};
    };

    class Poll {
    public:
        static void tryAdd(FDWaitContext *fdw) noexcept(false) {
            instance().doAdd(fdw);
        }

        typedef enum {
            POKE_NOOP = 1,
            POKE_STOP
        } Poke;

        static void remove(std::uint64_t fd) { instance().doRemove(fd); }

        static std::ptrdiff_t wait(
                std::span<AsyncResp> output,
                std::chrono::milliseconds timeout = -1ms) noexcept(false)
        { return instance().doWait(output, timeout.count()); }

        static void cancelTimer(TimerHandle timer)
        { instance().doCancelTimer(timer); }

        static TimerHandle addTimer(void *waiter, steady_clock::time_point dd)
        { return instance().doAddTimer(waiter, dd); }

        static void poll(std::chrono::milliseconds wait = -1ms);
        static void loop(std::chrono::milliseconds wait = -1ms);
        static bool armed(){ return instance().isArmed(); }
        static void poke(Poke cmd = POKE_NOOP)  { instance().doPoke(cmd);  }


    protected:
        static Poll& instance();
        virtual bool isArmed() const { return false; }
        virtual void doPoke(Poke cmd) = 0;
        virtual void doAdd(FDWaitContext* req) noexcept(false) = 0;
        virtual void doRemove(std::uint64_t fd) = 0;
        virtual std::ptrdiff_t doWait(
                    std::span<AsyncResp>& output,
                    int64_t timeout) noexcept(false) = 0;

        TimerHandle doAddTimer(void* waiter, steady_clock::time_point dd) noexcept;
        void doCancelTimer(TimerHandle timer);
        void fireExpiredTimers();
        int64_t pollWaitTimeout(int64_t waitMs);

        TimerList _timers{};
    };
}
