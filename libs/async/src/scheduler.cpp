/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-06
 */

#include "suil/async/scheduler.hpp"
#include "suil/async/fdwait.hpp"

#include <sys/eventfd.h>
#include <sys/epoll.h>

#ifndef SUIL_ASYNC_MAXIMUM_CONCURRENCY
#define SUIL_ASYNC_MAXIMUM_CONCURRENCY 64u
#endif

namespace suil {

    Scheduler::Scheduler()
    {
        mtid();
        _cpuCount = std::thread::hardware_concurrency();
        if (unlikely(_cpuCount > SUIL_ASYNC_MAXIMUM_CONCURRENCY)) {
            ASYNC_TRACE("[%zu]: async concurrency capped to %u", ASYNC_LOCATION, tid(), SUIL_ASYNC_MAXIMUM_CONCURRENCY);
            _cpuCount = SUIL_ASYNC_MAXIMUM_CONCURRENCY;
        }

        _cpuMask = (1 << (_cpuCount + 1)) - 1;
        ASYNC_TRACE("[%zu]: spawning scheduler with %u threads", ASYNC_LOCATION, tid(), _cpuCount);
        void *raw = operator new[](sizeof(detail::WorkQueue) * _cpuCount);
        _queues = static_cast<detail::WorkQueue *>(raw);
        for (auto i = 0; i != _cpuCount; i++) {
            new(_queues +i) detail::WorkQueue(*this, i);
        }

        _epfd = epoll_create1(EPOLL_CLOEXEC);
        SUIL_ASSERT(_epfd != INVALID_FD);

        _evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        SUIL_ASSERT(_evfd != INVALID_FD);

        struct epoll_event ev{.events = EPOLLERR | EPOLLIN | EPOLLHUP, .data = {.fd = _evfd}};
        int rc = epoll_ctl(_epfd, EPOLL_CTL_ADD, _evfd, &ev);
        SUIL_ASSERT(rc != -1);

        _eventThread = std::thread([this] {
            _active = true;
            while (_active) {
                // wait for events on the event loop
                Event* triggeredEvents[SUIL_ASYNC_MAXIMUM_CONCURRENCY] = {nullptr};
                auto events = waitForEvents({triggeredEvents, SUIL_ASYNC_MAXIMUM_CONCURRENCY});
                if (!_active) {
                    break;
                }

                for (auto i = 0u; i < events; i++) {
                    auto& event = triggeredEvents[i]->_handle;

                    if (event.timerHandle) {
                        cancelTimer(*event.timerHandle);
                        event.timerHandle = std::nullopt;
                    }

                    struct epoll_event ev{};
                    epoll_ctl(_epfd, EPOLL_CTL_DEL, event.fd, &ev);
                    schedule(event.coro, event.affinity, event.priority);
                }

                // check expired timers
                scheduleExpiredTimers();
            }

            if (_evfd != INVALID_FD) {
                ::close(_evfd);
                _evfd = INVALID_FD;
            }
            if (_epfd != INVALID_FD) {
                ::close(_epfd);
                _evfd = INVALID_FD;
            }
        });
    }

    void Scheduler::schedule(std::coroutine_handle<> coroutine,
                             uint64_t affinity,
                             uint32_t priority,
                             AsyncLocation location)
    {
        if (affinity == 0) {
            affinity = ~affinity & _cpuMask;
        }

        for (auto i = 0u; i != _cpuCount; i++) {
            if (_queues[i].size_approx() == 0) {
                ASYNC_TRACEn(2, "[%zu] scheduling coro into empty queue %u", ASYNC_LOCATION, tid(), i);
                _queues[i].enqueue(coroutine, priority, location);
                return;
            }
        }

        // All queues appear to be busy, pick a random one with reasonably low
        // discrepancy (Kronecker recurrence sequence)
        auto index = static_cast<uint32_t>(float(_update++) * std::numbers::phi_v<float>) % std::popcount(affinity);
        for (auto i = 0u; i != index; ++i) {
            affinity &= ~(1 << (std::countr_zero(affinity) +1));
        }

        auto queue = std::countr_zero(affinity);
        ASYNC_TRACEn(2, "[%zu] scheduling coro into queue %u", ASYNC_LOCATION, tid(), queue);
        _queues[queue].enqueue(coroutine, priority, location);
    }

    void Scheduler::relocate(std::coroutine_handle<> coroutine,
                             uint16_t cpu,
                             AsyncLocation location)
    {
        SUIL_ASSERT(cpu < _cpuCount);
        ASYNC_TRACEn(2, "[%zu] scheduling coro into empty queue %u", ASYNC_LOCATION, tid(), cpu);
        _queues[cpu].enqueue(coroutine, PRIO_0, location);
    }

    bool Scheduler::schedule(Event *event)
    {
        auto& handle = event->_handle;
        SUIL_ASSERT((handle.state != Event::esSCHEDULED) &&
                    (handle.fd != INVALID_FD) &&
                    (handle.coro != nullptr));

        int op = EPOLL_CTL_ADD, ec = 0;

        TRY_OP:
        struct epoll_event ev {
                .events = EPOLLHUP | EPOLLERR | (handle.ion == Event::IN ? EPOLLIN : EPOLLOUT),
                .data = {.ptr = event }
        };

        ec = epoll_ctl(_epfd, op,handle.fd, &ev);
        if (ec == 0) {
            if (handle.eventDeadline != steady_clock::time_point{}) {
                handle.timerHandle = addTimer({.timerDeadline = handle.eventDeadline, .targetEvent = event});
            }
            handle.state = Event::esSCHEDULED;
            return true;
        }
        if (errno == EEXIST) {
            // already added, modify
            op = EPOLL_CTL_MOD;
            goto TRY_OP;
        }

        return false;
    }

    void Scheduler::schedule(Timer *timer)
    {
        SUIL_ASSERT((timer->_state != Timer::tsSCHEDULED) &&
                    (timer->_coro != nullptr));
        timer->_state = Timer::tsSCHEDULED;
        timer->_handle =
                addTimer({.timerDeadline = now() + timer->_timeout, .targetTimer = timer});
    }

    void Scheduler::unschedule(Event *event)
    {
        auto& handle = event->_handle;
        auto state = Event::esSCHEDULED;
        if (handle.state.compare_exchange_weak(state, Event::esABANDONED)) {
            // allow only 1 thread to unschedule event
            if (handle.timerHandle) {
                cancelTimer(*handle.timerHandle);
                handle.timerHandle = std::nullopt;
            }
            struct epoll_event ev{};
            epoll_ctl(_epfd, EPOLL_CTL_DEL, handle.fd, &ev);
            schedule(handle.coro, handle.affinity, handle.priority);
        }
    }

    void Scheduler::unschedule(Timer *timer)
    {
        auto state = Timer::tsSCHEDULED;
        if (timer->_state.compare_exchange_weak(state, Timer::tsSCHEDULED)) {
            // only 1 thread allowed to unschedule event
            SUIL_ASSERT(timer->_state == Timer::tsSCHEDULED);
            if (timer->_handle) {
                cancelTimer(*timer->_handle);
                timer->_handle = std::nullopt;
            }
            schedule(timer->_coro, timer->_affinity, timer->_priority);
        }
    }

    std::size_t Scheduler::waitForEvents(std::span<Event *> resp)
    {
        struct epoll_event events[resp.size()];
        auto count = epoll_wait(_epfd,
                                events,
                                int(resp.size()),
                                computeWaitTimeout());

        if (count == -1) {
            SUIL_ASSERT(errno == EINTR);
        }

        int found{0};
        for (int i = 0; i < count; i++) {
            auto& event = events[i];
            if (event.data.fd == _evfd) {
                handleThreadEvent();
                continue;
            }

            auto& handle = static_cast<Event *>(events[i].data.ptr)->_handle;
            auto state = Event::esSCHEDULED;
            if (handle.state.compare_exchange_weak(state, Event::esFIRED)) {
                if (event.events & (EPOLLERR | EPOLLHUP)) {
                    handle.state = Event::esERROR;
                } else {
                    auto ev = (handle.ion == Event::IN ? EPOLLIN : EPOLLOUT);
                    SUIL_ASSERT((event.events & ev) == ev);
                    handle.state = Event::esFIRED;
                }
                resp[found++] = static_cast<Event *>(events[i].data.ptr);
            }
        }

        return found;
    }

    void Scheduler::handleThreadEvent() const
    {
        eventfd_t count{0};
        auto nrd = read(_evfd, &count, sizeof(count));
        SUIL_ASSERT(nrd == sizeof(count));
    }

    void Scheduler::signalThread()
    {
        bool expected = false;
        // only need to signal EPOLL once
        if (_threadSignaling.compare_exchange_weak(expected, true)) {
            eventfd_t count{0x01};
            auto nwr = ::write(_evfd, &count, sizeof(count));
            SUIL_ASSERT(nwr == 8);
            _threadSignaling = false;
        }
    }

    void Scheduler::cancelTimer(Timer::Handle handle)
    {
        std::lock_guard<std::mutex> lg(_timersLock);
        if (handle != _timers.end()) {
            _timers.erase(handle);
        }
    }

    Timer::Handle Scheduler::addTimer(Timer::Entry entry)
    {
        _timersLock.lock();
        auto it = _timers.insert(entry);
        _timersLock.unlock();

        signalThread();

        SUIL_ASSERT(it.second);
        return it.first;
    }

    void Scheduler::scheduleExpiredTimers()
    {
        auto tp = now();

        _timersLock.lock();
        while (!_timers.empty()) {
            auto it = _timers.begin();
            if (it->timerDeadline <= tp) {
                auto handle = *it;
                _timers.erase(it);
                _timersLock.unlock();
                if (handle.targetEvent) {
                    auto state = Event::esSCHEDULED;
                    auto& event = handle.targetEvent->_handle;
                    if (event.state.compare_exchange_weak(state, Event::esTIMEOUT)) {
                        event.timerHandle = std::nullopt;
                        struct epoll_event ev{};
                        epoll_ctl(_epfd, EPOLL_CTL_DEL, event.fd, &ev);
                        schedule(event.coro, event.affinity, event.priority);
                    }
                }
                else {
                    auto& timer = *handle.targetTimer;
                    auto state = Timer::tsSCHEDULED;
                    if (timer._state.compare_exchange_weak(state, Timer::tsFIRED)) {
                        timer._handle = std::nullopt;
                        schedule(timer._coro, timer._affinity, timer._affinity);
                    }
                }
                _timersLock.lock();
            } else {
                break;
            }
        }
        _timersLock.unlock();
    }

    int Scheduler::computeWaitTimeout()
    {
        auto it = _timers.begin();
        if (it != _timers.end()) {
            auto at = std::chrono::duration_cast<std::chrono::milliseconds>(it->timerDeadline - now()).count();
            if (at < 0) {
                return 0;
            }
            return int(at);
        }

        return -1;
    }

    Scheduler& Scheduler::instance() noexcept
    {
        static Scheduler scheduler;
        return scheduler;
    }

    void Scheduler::init()
    {
        auto maxWait = 6;
        while (!Scheduler::instance()._active && maxWait-- > 0) {
            std::this_thread::sleep_for(milliseconds{500});
        }
        SUIL_ASSERT(Scheduler::instance()._active);
    }

    void Scheduler::wait()
    {
        if (Scheduler::instance()._eventThread.joinable()) {
            Scheduler::instance()._eventThread.join();
        }
    }

    void Scheduler::abort()
    {
        bool isActive{true};
        if (Scheduler::instance()._active.compare_exchange_weak(isActive, false)) {
            Scheduler::instance().signalThread();
        }
    }

    Scheduler::~Scheduler() noexcept
    {
        _active = false;
        signalThread();
        if (_eventThread.joinable()) {
            _eventThread.join();
        }
        if (_queues != nullptr) {
            for (auto i = 0u; i != +_cpuCount; i++) {
                _queues[i].~WorkQueue();
            }
            operator delete[](static_cast<void *>(_queues));
            _queues = nullptr;
        }
    }
}