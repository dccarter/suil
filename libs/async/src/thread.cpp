/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-08
 */

#include "suil/async/thread.hpp"

namespace suil {

    void *ThreadJoiner::ThreadSpawn::resumeOnThread(void *coro)
    {
        auto task = std::coroutine_handle<void>::from_address(coro);
        if (!task.done()) {
            task.resume();
        }
        return task.address();
    }

    ThreadJoiner::operator pthread_t() const noexcept
    { return _promise->tid; }

    ThreadJoiner::~ThreadJoiner() noexcept(false)
    {
        pthread_t tid = *this;
        if (tid == pthread_t{})
            return;

        void *ptr{};
        if (auto ec = pthread_join(*this, &ptr)) {
            throw std::system_error{errno, std::system_category()};
        }
        if (auto frame = std::coroutine_handle<void>::from_address(ptr)) {
            frame.destroy();
        }
    }
}