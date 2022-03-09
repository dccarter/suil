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
 * @author modified by Carter
 * @date 2022-01-09
 *
 * from: https://github.com/lewissbaker/cppcoro/blob/a87e97fe5b6091ca9f6de4637736b8e0d8b109cf/lib/lightweight_manual_reset_event.cpp
 */

#include "suil/async/sync.hpp"

#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <cerrno>
#include <climits>

namespace  {
    // No futex() function provided by libc, wrap the syscall ourselves here.
    auto futex(int *userAddress, int futexOperation, int Value)
    {
        return syscall(
                SYS_futex,
                userAddress,
                futexOperation,
                Value,
                0,
                0,
                0);
    }
}
namespace suil {

    detail::lightweight_manual_reset_event::lightweight_manual_reset_event(bool initiallySet)
            : _value(initiallySet ? 1 : 0) {}

    detail::lightweight_manual_reset_event::~lightweight_manual_reset_event() = default;

    void detail::lightweight_manual_reset_event::set() noexcept
    {
        _value.store(1, std::memory_order_release);

        constexpr int numberOfWaitersToWakeUp = INT_MAX;

        [[maybe_unused]] auto numberOfWaitersWokenUp = futex(
                reinterpret_cast<int *>(&_value),
                FUTEX_WAKE_PRIVATE,
                numberOfWaitersToWakeUp);

        SUIL_ASSERT(numberOfWaitersWokenUp != -1);
    }

    void detail::lightweight_manual_reset_event::reset() noexcept
    {
        _value.store(0, std::memory_order_relaxed);
    }

    void detail::lightweight_manual_reset_event::wait() noexcept {
        // Wait in a loop as futex() can have spurious wake-ups.
        int oldValue = _value.load(std::memory_order_acquire);
        while (oldValue == 0) {
            auto result = futex(
                    reinterpret_cast<int *>(&_value),
                    FUTEX_WAIT_PRIVATE,
                    oldValue);
            if (result == -1) {
                if (errno == EAGAIN) {
                    // The state was changed from zero before we could wait.
                    // must have been changed to 1.
                    return;
                }

                // Other errors we'll treat as transient and just read the
                // value and go around the loop again.
            }
            oldValue = _value.load(std::memory_order_acquire);
        }
    }
}