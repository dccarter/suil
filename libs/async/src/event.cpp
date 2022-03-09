// Copyright 2017 Lewis Baker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is furnished
// to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Modified by Carter
 * @date 2022-01-09
 */

#include <suil/async/event.hpp>

#include <algorithm>

namespace {
    constexpr std::uint64_t SET_INCREMENT = 1;
    constexpr std::uint64_t WAITER_INCREMENT = std::uint64_t{1} << 32;

    constexpr std::uint32_t get_set_count(std::uint64_t state)
    {
        return static_cast<std::uint32_t>(state);
    }

    constexpr std::uint32_t get_waiter_count(std::uint64_t state)
    {
        return static_cast<std::uint32_t>(state >> 32);
    }

    constexpr std::uint32_t get_resumable_waiter_count(std::uint64_t state)
    {
        return std::min(get_waiter_count(state), get_set_count(state));
    }
}

namespace suil {

    AutoResetEvent::AutoResetEvent(bool initiallySet) noexcept
        : _state{initiallySet? SET_INCREMENT: 0},
          _newWaiters{nullptr},
          _waiters{nullptr}
    {}

    AutoResetEvent::~AutoResetEvent()
    {
        SUIL_ASSERT(_newWaiters.load(std::memory_order_relaxed) == nullptr);
        SUIL_ASSERT(_waiters == nullptr);
    }

    AutoResetEvent::auto_reset_event_operation AutoResetEvent::operator co_await() const noexcept
    {
        auto oldState = _state.load(std::memory_order_relaxed);
        if (get_set_count(oldState) > get_waiter_count(oldState)) {
            // try to synchronously acquire the event
            auto acquired = _state.compare_exchange_strong(oldState,
                                                           oldState - SET_INCREMENT,
                                                           std::memory_order_acquire,
                                                           std::memory_order_relaxed);
            if (acquired) {
                return auto_reset_event_operation{};
            }
        }
        return auto_reset_event_operation{};
    }

    void AutoResetEvent::set() noexcept
    {
        auto oldState = _state.load(std::memory_order_relaxed);
        do {
            if (get_set_count(oldState) > get_waiter_count(oldState)) {
                return;
            }
        } while (!_state.compare_exchange_weak(oldState,
                                              oldState + SET_INCREMENT,
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire));

        if (oldState != 0 && get_set_count(oldState) == 0) {
            resumeWaiters(oldState + SET_INCREMENT);
        }
    }

    void AutoResetEvent::reset() noexcept
    {
        auto oldState = _state.load(std::memory_order_relaxed);
        while (get_set_count(oldState) > get_waiter_count(oldState)) {
            auto eventReset = _state.compare_exchange_weak(oldState,
                                                           oldState - SET_INCREMENT,
                                                           std::memory_order_relaxed);
            if (eventReset) {
                return;
            }
        }
    }

    void AutoResetEvent::resumeWaiters(std::uint64_t initState) const noexcept
    {
        auto_reset_event_operation* waitersToResume{nullptr};
        auto_reset_event_operation** waitersToResumeEnd{&waitersToResume};
        std::uint32_t waitersToResumeCount = get_resumable_waiter_count(initState);
        SUIL_ASSERT(waitersToResumeCount > 0);

        do {
            for (std::uint32_t i = 0; i < waitersToResumeCount; ++i) {
                if (_waiters == nullptr) {
                    auto newWaiters = _newWaiters.exchange(nullptr, std::memory_order_relaxed);
                    SUIL_ASSERT(newWaiters != nullptr);

                    do {
                        auto next = newWaiters->_next;
                        newWaiters->_next = _waiters;
                        _waiters = newWaiters;
                        newWaiters = next;
                    } while (newWaiters != nullptr);
                }

                // pop next waiter from list
                auto waiterToResume = _waiters;
                _waiters = _waiters->_next;
                // Put it onto the end of the list of waiters to resume
                waitersToResume->_next = nullptr;
                *waitersToResumeEnd = waiterToResume;
                waitersToResumeEnd = &waitersToResume->_next;
            }

            const auto delta = std::uint64_t(waitersToResumeCount) |
                                                std::uint64_t(waitersToResumeCount) << 32;
            const auto newState = _state.fetch_sub(delta, std::memory_order_acq_rel) - delta;

            waitersToResumeCount = get_resumable_waiter_count(newState);
        } while (waitersToResumeCount > 0);

        // resume all dequeued waiters
        SUIL_ASSERT(waitersToResume != nullptr);
        do {
            auto* const waiter = waitersToResume;
            auto* const next = waiter->_next;
            if (waiter->_refCount.fetch_sub(1, std::memory_order_release) == 1) {
                waiter->_awaiter.resume();
            }
            waitersToResume = next;
        } while (waitersToResume != nullptr);
    }

    AutoResetEvent::auto_reset_event_operation::auto_reset_event_operation(const AutoResetEvent &event) noexcept
        : _event{&event},
          _refCount{2}
    {}

    AutoResetEvent::auto_reset_event_operation::auto_reset_event_operation(
            const auto_reset_event_operation &other) noexcept
        : _event{other._event},
          _refCount{2}
    {}

    bool AutoResetEvent::auto_reset_event_operation::await_suspend(std::coroutine_handle<> awaiter) noexcept
    {
        _awaiter = awaiter;
        // queue the waiter to the list of new waiters
        auto head = _event->_newWaiters.load(std::memory_order_relaxed);
        do {
            _next = head;
        } while (!_event->_newWaiters.compare_exchange_weak(head,
                                                            this,
                                                            std::memory_order_release,
                                                            std::memory_order_relaxed));

        auto oldState = _event->_state.fetch_add(WAITER_INCREMENT, std::memory_order_relaxed);
        if (oldState != 0 && get_waiter_count(oldState) == 0) {
            _event->resumeWaiters(oldState + WAITER_INCREMENT);
        }

        return _refCount.fetch_sub(1, std::memory_order_acquire) != 1;
    }

    ManualResetEvent::ManualResetEvent(bool initiallySet) noexcept
        : _state{initiallySet? static_cast<void *>(this) : nullptr}
    {}

    ManualResetEvent::~ManualResetEvent()
    {
        SUIL_ASSERT(_state.load(std::memory_order_relaxed) == nullptr ||
               _state.load(std::memory_order_relaxed) == static_cast<const void*>(this));
    }

    bool ManualResetEvent::isSet() const noexcept
    {
        return _state.load(std::memory_order_relaxed) == static_cast<const void*>(this);
    }

    ManualResetEvent::manual_reset_event_operation ManualResetEvent::operator co_await() const noexcept
    {
        return manual_reset_event_operation{*this};
    }

    void ManualResetEvent::set() noexcept
    {
        auto const setState = static_cast<void *>(this);

        auto oldState = _state.exchange(setState, std::memory_order_acq_rel);
        if (oldState != setState) {
            auto current = static_cast<manual_reset_event_operation*>(oldState);
            while (current != nullptr) {
                auto next = current->_next;
                current->_awaiter.resume();
                current = next;
            }
        }
    }

    void ManualResetEvent::reset() noexcept
    {
        auto oldState = static_cast<void *>(this);
        _state.compare_exchange_weak(oldState, nullptr, std::memory_order_relaxed);
    }

    ManualResetEvent::manual_reset_event_operation::manual_reset_event_operation(
            const ManualResetEvent &event) noexcept
        : _event{event}
    {}

    bool ManualResetEvent::manual_reset_event_operation::await_ready() const noexcept
    {
        return _event.isSet();
    }

    bool ManualResetEvent::manual_reset_event_operation::await_suspend(std::coroutine_handle<> awaiter) noexcept
    {
        _awaiter = awaiter;
        auto const setState = static_cast<const void *>(&_event);
        auto oldState = _event._state.load(std::memory_order_acquire);
        do {
            if (oldState == setState) {
                return false;
            }
            _next = static_cast<manual_reset_event_operation*>(oldState);
        } while (!_event._state.compare_exchange_weak(oldState,
                                                     static_cast<void *>(this),
                                                     std::memory_order_release,
                                                     std::memory_order_acquire));
        return true;
    }
}
