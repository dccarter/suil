/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-07
 */

#include "suil/async/poll.hpp"

namespace suil {

    void Poll::poll(std::chrono::milliseconds wait)
    {
        constexpr auto POLL_COUNT{32u};
        AsyncResp resp[POLL_COUNT] = {{}};

        auto count = Poll::wait(
                resp,
                std::chrono::duration_cast<std::chrono::milliseconds>(wait));

        for (auto i = 0u; i < count; i++) {
            if (auto req = (FDWaitContext *)(resp[i].ptr)) {
                req->fdw = resp[i].res;

                if (req->waitTimer) {
                    cancelTimer(*req->waitTimer);
                    req->waitTimer = std::nullopt;
                }

                if (auto coro = std::coroutine_handle<>::from_address(req->waitingHandle)) {
                    if (!coro.done()) {
                        coro.resume();
                    }
                }
            }
        }

        // attempt to fire any expired timers
        instance().fireExpiredTimers();
    }

    void Poll::loop(std::chrono::milliseconds wait)
    {
        while (Poll::armed()) {
            Poll::poll(wait);
        }
    }

    TimerHandle Poll::doAddTimer(void *waiter, steady_clock::time_point dd) noexcept
    {
        if (dd == steady_clock::time_point{}) {
            return TimerHandle{};
        }
        return _timers.emplace(Timer{dd, now(), waiter}).first;
    }

    void Poll::doCancelTimer(TimerHandle timer)
    {
        if (timer != _timers.end()) {
            _timers.erase(timer);
        }
    }

    void Poll::fireExpiredTimers()
    {
        auto tp = now();
        auto it = _timers.begin();

        while (it != _timers.end()) {
            if (it->fire <= tp) {
                auto target = it->target;

                it = _timers.erase(it);
                if (auto coro = std::coroutine_handle<void>::from_address(target)) {
                    if (!coro.done()) {
                        coro.resume();
                    }
                }
            }
            else {
                break;
            }
        }
    }

    int64_t Poll::pollWaitTimeout(int64_t waitMs)
    {
        auto it = _timers.begin();
        if (it != _timers.end()) {
            auto at = std::chrono::duration_cast<std::chrono::milliseconds>(it->fire - now()).count();
            if (at < 0) {
                return 0;
            }
            else if (waitMs > 0) {
                return std::min(at, waitMs);
            }
            else {
                return at;
            }
        }

        return waitMs;
    }
}