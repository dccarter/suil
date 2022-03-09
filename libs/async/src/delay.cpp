/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author Carter
 * @date 2022-03-06
 */

#include "suil/async/delay.hpp"
#include "suil/async/scheduler.hpp"

namespace suil {

    Timer::Timer(milliseconds timeout, uint16_t affinity, uint16_t priority)
            : _timeout{timeout}, _affinity{uint16(affinity)}, _priority{uint16(priority)}
    {}

    Timer::Timer(Timer &&other) noexcept
    {
        Ego = std::move(other);
    }

    Timer& Timer::operator=(Timer &&other) noexcept
    {
        if (this == &other) {
            SUIL_ASSERT(_state == tsCREATED);
            _state = other._state.exchange(tsABANDONED);
            _timeout = std::exchange(other._timeout, -1ms);
            _coro = std::exchange(other._coro, nullptr);
            _affinity = std::exchange(other._affinity, 0);
            _priority = std::exchange(other._priority, PRIO_1);
        }

        return Ego;
    }

    Timer::~Timer() noexcept
    {
        SUIL_ASSERT(_state == tsCREATED);
    }

    void Timer::await_suspend(std::coroutine_handle<> coroutine) noexcept
    {
        SUIL_ASSERT(_state == tsCREATED);
        _coro = coroutine;
        Scheduler::instance().schedule(this);
    }

    bool Timer::Entry::operator<(const Entry &rhs) const
    {
        return (timerDeadline < rhs.timerDeadline) ||
               ((timerDeadline == rhs.timerDeadline) &&
                ((targetEvent != nullptr && targetEvent != rhs.targetEvent) ||
                 (targetTimer != nullptr && targetTimer != rhs.targetTimer)));
    }
}