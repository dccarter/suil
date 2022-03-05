/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-09
 */

#pragma once

#include <suil/async/coroutine.hpp>

#include <atomic>
#include <cstdint>

namespace suil {

    class AutoResetEvent final {
    public:
        struct auto_reset_event_operation;
        AutoResetEvent(bool initiallySet = false) noexcept;
        ~AutoResetEvent();

        auto_reset_event_operation operator co_await() const noexcept;

        void set() noexcept;

        void reset() noexcept;

    private:
        friend struct auto_reset_event_operation;
        void resumeWaiters(std::uint64_t initState) const noexcept;

        mutable std::atomic<std::uint64_t> _state{};
        mutable std::atomic<auto_reset_event_operation*> _newWaiters{};
        mutable auto_reset_event_operation* _waiters{};
    };

    struct AutoResetEvent::auto_reset_event_operation {
        auto_reset_event_operation() noexcept = default;
        explicit auto_reset_event_operation(const AutoResetEvent& event) noexcept;
        auto_reset_event_operation(const auto_reset_event_operation& other) noexcept;

        bool await_ready() const noexcept { return _event == nullptr; }
        bool await_suspend(std::coroutine_handle<> awaiter) noexcept;
        void await_resume() const noexcept {}

    private:
        friend class AutoResetEvent;
        const AutoResetEvent* _event{};
        auto_reset_event_operation* _next{};
        std::coroutine_handle<> _awaiter{};
        std::atomic<std::uint32_t> _refCount{};
    };

    class ManualResetEvent final  {
    public:
        struct manual_reset_event_operation;
        ManualResetEvent(bool initiallySet = false) noexcept;
        ~ManualResetEvent();

        manual_reset_event_operation operator co_await() const noexcept;

        bool isSet() const noexcept;
        void set() noexcept;
        void reset() noexcept;

    private:
        friend struct manual_reset_event_operation;
        mutable std::atomic<void*> _state{};
    };

    struct ManualResetEvent::manual_reset_event_operation {
        explicit manual_reset_event_operation(const ManualResetEvent& event) noexcept;

        bool await_ready() const noexcept;
        bool await_suspend(std::coroutine_handle<> awaiter) noexcept;
        void await_resume() const noexcept {}

    private:
        friend class ManualResetEvent;
        const ManualResetEvent& _event;
        manual_reset_event_operation* _next{};
        std::coroutine_handle<> _awaiter{};
    };
}