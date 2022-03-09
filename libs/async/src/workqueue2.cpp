/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-05
 */

#include "suil/async/detail/work_queue.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>

namespace suil::detail {

    WorkQueue::WorkQueue(Scheduler& scheduler, uint32_t id)
            : scheduler_{scheduler}
            , id_{id}
            , sem_{0}
    {
        snprintf(label_, sizeof(label_), "work_queue:%i", id);
        active_ = true;
        thread_ = std::thread([this] {
#if defined(_WIN32)
            SetThreadAffinityMask(
            GetCurrentThread(), static_cast<uint32_t>(1ull << id_));
#elif defined(__linux__)
            // TODO: Android implementation
            pthread_t thread = pthread_self();
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(id_, &cpuset);
            int result = pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset);

            SUIL_ASSERT(result == 0);
#elif (__APPLE__)
            // TODO: MacOS/iOS implementation
#endif
            ASYNC_TRACE("[%zu] queue #%d created", ASYNC_LOCATION, tid(), id_);

            while (true)
            {
                sem_.acquire();

                if (!active_) {
                    return;
                }

                for (int i = SUIL_ASYNC_PRIORITY_COUNT - 1; i >= 0;) {
                    auto q = queues_[i].dequeue();
                    if (q.empty()) {
                        i--;
                        continue;
                    }

                    while (!q.empty()) {
                        qsize_--;
                        std::coroutine_handle<> coroutine = q.front();
                        q.pop();
                        ASYNC_TRACEn(2, "[%zu]: de-queueing coroutine %p (%u)\n",
                                     ASYNC_LOCATION,
                                     tid(),
                                     coroutine.address(),
                                     id_);
                        coroutine.resume();
                    }
                }
                // TODO: Implement some sort of work stealing here
            }
        });
    }

    WorkQueue::~WorkQueue() noexcept
    {
        active_ = false;
        sem_.release();
        thread_.join();
    }

    void WorkQueue::enqueue(std::coroutine_handle<> coroutine,
                               uint32_t priority,
                               AsyncLocation location)
    {
        priority = std::clamp<uint32_t>(priority, 0, SUIL_ASYNC_PRIORITY_COUNT - 1);
        ASYNC_TRACEn(2, "[%zu]: enqueueing coroutine %p", location, tid(), coroutine.address());
        {
            std::lock_guard<std::mutex> lg{queues_[priority].mutex};
            queues_[priority].Q.push(coroutine);
            qsize_++;
        }

        sem_.release();
    }

    std::queue<std::coroutine_handle<>> WorkQueue::Queue::dequeue()
    {
        std::lock_guard<std::mutex> lg{mutex};
        return std::exchange(Q, {});
    }

}