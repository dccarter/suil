/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-01-07
 */

#pragma once

#include <suil/async/coroutine.hpp>

#include <set>

namespace suil {

    struct Timer {
        std::chrono::steady_clock::time_point fire{}, added{};
        void* target{};

        bool operator<(const Timer& rhs) const {
            return (fire < rhs.fire) ||
                   (target != rhs.target && fire == rhs.fire);
        }

        bool operator<=(const std::chrono::steady_clock::time_point& tp) const {
            return fire <= tp;
        }
    };

    using TimerList = std::set<Timer>;
    using TimerHandle = TimerList::iterator;

    struct Delay {
        std::chrono::milliseconds wait{};
        void* waiter{};

        Delay() = default;
        Delay(std::chrono::milliseconds wait) noexcept;
        Delay(Delay&& o) noexcept;
        Delay& operator=(Delay&& o) noexcept;
        ~Delay() noexcept(false);

        Delay(const Delay&) = delete;
        Delay& operator=(const Delay&) = delete;

        bool await_ready() const noexcept { return wait.count() == 0; }

        void await_suspend(std::coroutine_handle<> h) noexcept(false);

        void await_resume() {}
    };

    inline auto now() -> std::chrono::steady_clock::time_point {
        return std::chrono::steady_clock::now();
    }

    inline auto nowms() -> int64_t {
        return std::chrono::duration_cast<std::chrono::milliseconds>(now().time_since_epoch()).count();
    }

    inline Delay delay(std::chrono::milliseconds d) {
        return Delay{d};
    }
}