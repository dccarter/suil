/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-06
 */

#pragma once

#include <suil/async/detail/concurrentqueue.h>
#include <suil/async/detail/work_queue.hpp>

#include <suil/async/coroutine.hpp>
#include <suil/async/delay.hpp>

#include <span>

namespace suil {
    struct Event;

    template<typename S>
    concept SchedulerInf = requires(S scheduler,
                                 std::coroutine_handle<> coroutine,
                                 uint64_t cpu_affinity,
                                 uint32_t priority,
                                 AsyncLocation location)
    {
        scheduler.schedule(coroutine, cpu_affinity, priority, location);
    };

    /**
     * Implement the Scheduler concept above to use your own coroutine scheduler
     */
    class Scheduler final {
    public:

        // Returns the default global thread-safe scheduler
        static Scheduler& instance() noexcept;
        static void init();
        void wait();
        static void abort();

        Scheduler();

        ~Scheduler() noexcept;

        DISABLE_COPY(Scheduler);
        DISABLE_MOVE(Scheduler);

        // Schedules a coroutine to be resumed at a later time as soon as a thread
        // is available. If you wish to provide your own custom scheduler, you can
        // schedule the coroutine in a single-threaded context, or with different
        // runtime behavior.
        //
        // In addition, you are free to handle or ignore the cpu affinity and
        // priority parameters differently. The default scheduler here supports TWO
        // priorities: 0 and 1. Coroutines with priority 1 will (in a best-effort
        // sense), be scheduled ahead of coroutines with priority 0.
        void schedule(std::coroutine_handle<> coroutine,
                      uint64_t affinity = 0,
                      uint32_t priority = 0,
                      AsyncLocation source_location = {});

        void relocate(std::coroutine_handle<> coroutine,
                      uint16_t  cpu = 0,
                      AsyncLocation source_location = {});

        bool schedule(Event* event);
        void schedule(Timer* event);

        void unschedule(Event* event);
        void unschedule(Timer* event);

    private:
        friend class detail::WorkQueue;

        std::size_t waitForEvents(std::span<Event*> resp);
        void cancelTimer(Timer::Handle handle);
        Timer::Handle addTimer(Timer::Entry entry);
        void scheduleExpiredTimers();
        int computeWaitTimeout();
        void handleThreadEvent() const;
        void signalThread();

        std::thread _eventThread;
        Timer::List _timers;
        std::mutex _timersLock;

        std::atomic<bool> _active{false};
        // Allocated as an array. One queue is assigned to each CPU
        detail::WorkQueue *_queues{nullptr};
        // Used to perform a low-discrepancy selection of work queue to enqueue a
        // coroutine to
        std::atomic<uint32_t> _update{0};
        // Specifically, this is the number of concurrent threads possible, which
        // may be double the physical CPU count if hyper threading or similar
        // technology is enabled
        uint32_t _cpuCount{0};
        uint32_t _cpuMask{0};
        int  _epfd{INVALID_FD};
        int  _evfd{INVALID_FD};
        std::atomic_bool _threadSignaling{false};
    };
}