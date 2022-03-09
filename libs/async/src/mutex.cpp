// Copyright 2017 Lewis Baker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//         of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
//         to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//         copies of the Software, and to permit persons to whom the Software is furnished
//         to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
//         copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//         AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author modified by Carter from https://github.com/lewissbaker/cppcoro/blob/master/lib/async_mutex.cpp
 * @date 2022-01-09
 */

#include "suil/async/mutex.hpp"

namespace suil {

    AsyncMutex::~AsyncMutex()
    {
        auto state = _state.load(std::memory_order_relaxed);
        SUIL_ASSERT(state == NOT_LOCKED || state == LOCKED_NO_WAITERS);
        SUIL_ASSERT(_waiters == nullptr);
    }

    bool AsyncMutex::tryLock() noexcept
    {
        auto oldState = NOT_LOCKED;
        return _state.compare_exchange_strong(oldState,
                                              LOCKED_NO_WAITERS,
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed);
    }

    AsyncMutex::lock_operation AsyncMutex::lockAsync() noexcept
    {
        return  AsyncMutex::lock_operation{ *this };
    }

    AsyncMutex::scoped_lock_operation AsyncMutex::scopedLockAsync() noexcept
    {
        return AsyncMutex::scoped_lock_operation{ *this };
    }

    void AsyncMutex::unlock()
    {
        SUIL_ASSERT(_state.load(std::memory_order_relaxed) != NOT_LOCKED);

        auto waitersHeader = _waiters;
        if (_waiters == nullptr) {
            auto oldState = LOCKED_NO_WAITERS;
            const bool lockReleased = _state.compare_exchange_strong(oldState,
                                                                     NOT_LOCKED,
                                                                     std::memory_order_release,
                                                                     std::memory_order_relaxed);
            if (lockReleased) {
                return;
            }

            oldState = _state.exchange(LOCKED_NO_WAITERS, std::memory_order_acquire);
            SUIL_ASSERT(oldState != LOCKED_NO_WAITERS && oldState != NOT_LOCKED);

            auto next = reinterpret_cast<AsyncMutex::lock_operation*>(oldState);
            do {
                auto tmp = next->_next;
                next->_next = waitersHeader;
                waitersHeader = next;
                next = tmp;
            } while (next != nullptr);
        }

        SUIL_ASSERT(waitersHeader != nullptr);
        // remove the next waiting coroutine
        _waiters = waitersHeader->_next;
        // resume the waiter
        waitersHeader->_waiter.resume();
    }

    bool AsyncMutex::lock_operation::await_suspend(std::coroutine_handle<> awaiter) noexcept
    {
        _waiter = awaiter;
        auto oldState = _mutex._state.load(std::memory_order_acquire);
        while (true) {
            if (oldState == NOT_LOCKED) {
                auto lockAcquired = _mutex._state.compare_exchange_weak(oldState,
                                                                        LOCKED_NO_WAITERS,
                                                                        std::memory_order_acquire,
                                                                        std::memory_order_relaxed);
                if (lockAcquired) {
                    // do not suspend
                    return false;
                }
            }
            else {
                // add the waiter into list of awaiting tasks
                _next = reinterpret_cast<AsyncMutex::lock_operation*>(oldState);
                auto waiterAdded = _mutex._state.compare_exchange_weak(oldState,
                                                                       reinterpret_cast<std::uintptr_t>(this),
                                                                       std::memory_order_release,
                                                                       std::memory_order_relaxed);
                if (waiterAdded) {
                    // added to waiters list, suspend now
                    return true;
                }
            }
        }
    }

}