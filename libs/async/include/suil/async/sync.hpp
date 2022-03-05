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
#include <exception>
#include <cstdint>
#include <utility>

namespace suil {
    namespace detail {

        struct lightweight_manual_reset_event {
            lightweight_manual_reset_event(bool initiallySet = false);
            ~lightweight_manual_reset_event();
            void set() noexcept;
            void reset() noexcept;
            void wait() noexcept;

        private:
            std::atomic<int> _value{};
        };

        template <typename T>
        struct sync_wait_task;

        template <typename R>
        class sync_wait_task_promise {
            using coroutine_handle_t = std::coroutine_handle<sync_wait_task_promise<R>>;
        public:
            using reference = R&&;
            sync_wait_task_promise() noexcept = default;

            void start(detail::lightweight_manual_reset_event& event)
            {
                _event = &event;
                coroutine_handle_t::from_promise(*this).resume();
            }

            auto get_return_object() noexcept {
                return coroutine_handle_t::from_promise(*this);
            }

            auto initial_suspend() noexcept -> std::suspend_always { return {}; }

            auto final_suspend() noexcept {
                struct completion_notifier {
                    bool await_ready() const noexcept { return  false; }
                    void await_suspend(coroutine_handle_t coro) const noexcept {
                        coro.promise()._event->set();
                    }
                    void await_resume() noexcept {}
                };

                return completion_notifier{};
            }

            auto yield_value(reference result) noexcept {
                _result = std::addressof(result);
                return final_suspend();
            }

            void return_void() noexcept {
            }

            void unhandled_exception() noexcept {
                _exception = std::current_exception();
            }

            reference result() {
                if (_exception) {
                    std::rethrow_exception(_exception);
                }
                return static_cast<reference>(*_result);
            }

        private:
            lightweight_manual_reset_event *_event{};
            std::remove_reference_t<R>* _result{};
            std::exception_ptr _exception{};
        };

        template<>
        struct sync_wait_task_promise<void> {
            using coroutine_handle_t = std::coroutine_handle<sync_wait_task_promise<void>>;

            sync_wait_task_promise() = default;

            void start(detail::lightweight_manual_reset_event& event) {
                _event = &event;
                coroutine_handle_t::from_promise(*this).resume();
            }

            auto get_return_object() noexcept {
                return coroutine_handle_t::from_promise(*this);
            }

            std::suspend_always initial_suspend() noexcept {
                return{};
            }

            auto final_suspend() noexcept {
                struct completion_notifier {
                    bool await_ready() const noexcept { return false; }
                    void await_suspend(coroutine_handle_t coroutine) const noexcept {
                        coroutine.promise()._event->set();
                    }
                    void await_resume() noexcept {}
                };

                return completion_notifier{};
            }

            void return_void() {}

            void unhandled_exception() {
                _exception = std::current_exception();
            }

            void result() {
                if (_exception) {
                    std::rethrow_exception(_exception);
                }
            }

        private:
            detail::lightweight_manual_reset_event* _event{};
            std::exception_ptr _exception{};

        };

        template<typename R>
        struct  sync_wait_task final
        {
            using promise_type = sync_wait_task_promise<R>;

            using coroutine_handle_t = std::coroutine_handle<promise_type>;

            explicit sync_wait_task(coroutine_handle_t coroutine) noexcept
                : _coroutine(coroutine)
            {}

            sync_wait_task(sync_wait_task&& other) noexcept
                : _coroutine(std::exchange(other.m_coroutine, coroutine_handle_t{}))
            {}

            ~sync_wait_task() {
                if (_coroutine) _coroutine.destroy();
            }

            sync_wait_task(const sync_wait_task&) = delete;
            sync_wait_task& operator=(const sync_wait_task&) = delete;

            void start(lightweight_manual_reset_event& event) noexcept
            {
                _coroutine.promise().start(event);
            }

            decltype(auto) result() {
                return _coroutine.promise().result();
            }

        private:
            coroutine_handle_t _coroutine;
        };

        template <typename T, typename R = typename awaitable_traits<T>::await_result_t>
            requires (!std::is_void_v<R>)
        auto make_sync_wait_task(T&& awaitable) -> sync_wait_task<R> {
            co_yield co_await std::forward<T>(awaitable);
        }

        template <typename T, typename R = typename awaitable_traits<T>::await_result_t>
            requires std::is_void_v<R>
        auto make_sync_wait_task(T&& awaitable) -> sync_wait_task<R> {
            co_await std::forward<T>(awaitable);
        }
    }

    template <typename T>
    auto syncWait(T&& awaitable) -> typename awaitable_traits<T&&>::await_result_t
    {
        auto task = detail::make_sync_wait_task(std::forward<T>(awaitable));
        detail::lightweight_manual_reset_event event{};
        task.start(event);
        event.wait();
    }
}