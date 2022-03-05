/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-01-07
 */


#include "suil/async/delay.hpp"
#include "suil/async/poll.hpp"

namespace suil {

    Delay::Delay(std::chrono::milliseconds wait) noexcept
        : wait{wait}
    {}

    Delay::Delay(Delay &&o) noexcept
        : wait{std::exchange(o.wait, 0ms)},
          waiter{std::exchange(o.waiter, nullptr)}
    {}

    Delay &Delay::operator=(Delay &&o) noexcept
    {
        wait = std::exchange(o.wait, 0ms);
        waiter = std::exchange(o.waiter, nullptr);
        return *this;
    }

    Delay::~Delay() noexcept(false)
    { waiter = nullptr; }

    void Delay::await_suspend(std::coroutine_handle<> h) noexcept(false)
    {
        waiter = h.address();
        Poll::addTimer(waiter, after(wait));
        Poll::poke(Poll::POKE_NOOP);
    }
}