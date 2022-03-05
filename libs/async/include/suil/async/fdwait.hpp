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

#include <suil/async/coroutine.hpp>
#include <suil/async/delay.hpp>

#include <optional>

namespace suil {
    
    enum FDW : std::uint16_t {
        FDW_ERR,
        FDW_IN,
        FDW_OUT,
        FDW_TIMEOUT
    };

    struct FDWaitContext final {
        int   fd{-1};
        FDW   fdw{FDW_ERR};
        void* waitingHandle{};
        steady_clock::time_point   waitDeadline{};
        std::optional<TimerHandle> waitTimer{std::nullopt};

        FDWaitContext() = default;
        FDWaitContext(int fd, FDW fdw, steady_clock::time_point deadline) noexcept;
        FDWaitContext(FDWaitContext&& o) noexcept;
        FDWaitContext& operator=(FDWaitContext&& o) noexcept;
        ~FDWaitContext() noexcept(false);

        FDWaitContext(const FDWaitContext&) = delete;
        FDWaitContext& operator=(const FDWaitContext&) = delete;

        bool await_ready() const noexcept { return this->ready(); }

        void await_suspend(std::coroutine_handle<> h) noexcept(false) {
            this->suspend(h);
        }

        bool ready() const noexcept;
        FDW resume() noexcept;
        void suspend(std::coroutine_handle<> h) noexcept(false);
    };

    inline auto fdwait(int fd, FDW fdw, steady_clock::time_point deadline = {}) {
        struct fd_awaitable {
            FDWaitContext wait{};

            fd_awaitable(int fd, FDW fdw, steady_clock::time_point deadline)
                : wait{fd, fdw, deadline}
            {}

            bool await_ready() const noexcept {
                return wait.ready();
            }

            void await_suspend(std::coroutine_handle<> h) noexcept(false) {
                wait.suspend(h);
            }

            FDW await_resume() noexcept {
                return wait.resume();
            }
        };

        return fd_awaitable{fd, fdw, deadline};
    }
}