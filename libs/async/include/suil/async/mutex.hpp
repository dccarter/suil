/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author modified by Carter from https://github.com/lewissbaker/cppcoro/blob/master/include/cppcoro/async_mutex.hpp
 * @date 2022-01-09
 *
 */

#pragma once

#include <suil/async/coroutine.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>

namespace suil {

    class AsyncMutex {
    public:
        struct lock_operation;
        struct scoped_lock_operation;

        AsyncMutex() = default;

        AsyncMutex(AsyncMutex&&) = default;
        AsyncMutex& operator=(AsyncMutex&&) = default;

        ~AsyncMutex();

        bool tryLock() noexcept;
        lock_operation lockAsync() noexcept;
        scoped_lock_operation scopedLockAsync() noexcept;
        void unlock();

    private:
        friend struct lock_operation;
        static constexpr std::uintptr_t NOT_LOCKED = 1;
        static constexpr std::uintptr_t LOCKED_NO_WAITERS = 0;
        std::atomic<std::uintptr_t> _state{NOT_LOCKED};
        lock_operation* _waiters{nullptr};
    };

    struct AsyncMutexLock final {
        explicit AsyncMutexLock(AsyncMutex& mutex, std::adopt_lock_t) noexcept
            : _mutex{&mutex}
        {}

        AsyncMutexLock(AsyncMutexLock&& other) noexcept
            : _mutex{std::exchange(other._mutex, nullptr)}
        {}

        AsyncMutexLock(const AsyncMutex&) = delete;
        AsyncMutexLock& operator=(const AsyncMutex&) = delete;

        ~AsyncMutexLock() {
            if (_mutex) {
                _mutex->unlock();
                _mutex = nullptr;
            }
        }

    private:
        AsyncMutex* _mutex;
    };

    struct AsyncMutex::lock_operation {
        explicit lock_operation(AsyncMutex& mutex) : _mutex{mutex}
        {}

        bool await_ready() const noexcept { return  false; }
        bool await_suspend(std::coroutine_handle<> awaiter) noexcept;
        void await_resume() {}

    protected:
        friend class AsyncMutex;
        AsyncMutex& _mutex;
    private:
        lock_operation *_next{nullptr};
        std::coroutine_handle<> _waiter{};
    };

    struct AsyncMutex::scoped_lock_operation final : AsyncMutex::lock_operation {
        using AsyncMutex::lock_operation::lock_operation;

        [[nodiscard]]
        AsyncMutexLock await_resume() const noexcept {
            return AsyncMutexLock{_mutex, std::adopt_lock};
        }
    };
}