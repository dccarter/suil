/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-06
 */

#pragma once

#include <suil/async/coroutine.hpp>
#include <atomic>
#include <cassert>
#include <exception>
#include <stdexcept>
#include <variant>

namespace suil {

    template <bool I, typename T>
    struct task;

    class broken_promise : public std::logic_error {
    public:
        broken_promise()
            : std::logic_error("broken promise")
        {}
    };

    namespace detail {
        template <bool I>
        using initial_suspend_t = std::conditional_t<I, std::suspend_always, std::suspend_never>;

        template <bool I = true>
        struct task_promise_base {
            friend struct final_awaitable;

            struct final_awaitable {
                bool await_ready() const noexcept { return false; }
                template <typename P>
                sxy_no_inline void await_suspend(std::coroutine_handle<P> coro) noexcept {
                    task_promise_base& promise = coro.promise();
                    if (promise._state.exchange(true, std::memory_order_acq_rel)) {
                        promise._continuation.resume();
                    }
                }
                void await_resume() noexcept {}
            };

            task_promise_base() : _state{false}
            {}

            auto initial_suspend() noexcept { return initial_suspend_t<I>{}; }

            auto final_suspend() noexcept { return  final_awaitable{}; }

            bool try_set_continuation(std::coroutine_handle<> continuation) {
                _continuation = continuation;
                return !_state.exchange(true, std::memory_order_acq_rel);
            }

        private:
            std::atomic<bool> _state{};
            std::coroutine_handle<> _continuation{};
        };

        template <bool I, typename T>
        class task_promise : public task_promise_base<I> {
        public:
            task_promise() noexcept = default;
            ~task_promise() {
                _value = {};
            }

            task<I, T> get_return_object() noexcept;

            void unhandled_exception() noexcept {
                _value = std::current_exception();
            }

            template<typename V>
                requires std::is_convertible_v<V&&, T>
            void return_value(V&& value) noexcept(std::is_nothrow_constructible_v<T, V&&>) {
                _value = std::forward<V>(value);
            }

            T& result() & {
                if (std::holds_alternative<std::exception_ptr>(_value)) {
                    std::rethrow_exception(std::get<std::exception_ptr>(_value));
                }
                else {
                    SXY_ASSERT(std::holds_alternative<T>(_value));
                    return std::get<T>(_value);
                }
            }

            using rvalue_type = std::conditional_t<std::is_arithmetic_v<T> || std::is_pointer_v<T>, T, T&&>;

            rvalue_type result() && {
                if (std::holds_alternative<std::exception_ptr>(_value)) {
                    std::rethrow_exception(std::get<std::exception_ptr>(_value));
                }
                else {
                    assert(std::holds_alternative<T>(_value));
                    return std::get<T>(std::move(_value));
                }
            }

        private:
            std::variant<std::nullptr_t, T,std::exception_ptr> _value{nullptr};
        };

        template<bool I>
        class task_promise<I, void> : public task_promise_base<I> {
        public:
            task_promise() noexcept = default;

            task<I, void> get_return_object() noexcept;

            void return_void() noexcept {}

            void unhandled_exception() noexcept {
                _exception = std::current_exception();
            }

            void result() {
                if (_exception) {
                    std::rethrow_exception(_exception);
                }
            }

        private:
            std::exception_ptr _exception{nullptr};
        };

        template<bool I, typename T>
        class task_promise<I, T&> : public task_promise_base<I> {
        public:
            task_promise() noexcept = default;

            task<I, T&> get_return_object() noexcept;

            void unhandled_exception() noexcept {
                _exception = std::current_exception();
            }

            void return_value(T& value) noexcept {
                _value = std::addressof(value);
            }

            T& result() {
                if (_exception) {
                    std::rethrow_exception(_exception);
                }
                return *_value;
            }

        private:
            T* _value{nullptr};
            std::exception_ptr _exception{nullptr};
        };
    }

    template<bool I, typename T = void>
    class [[nodiscard]] task {
    public:
        using promise_type = detail::task_promise<I, T>;
        using value_type = T;

    private:

        struct awaitable_base {
            std::coroutine_handle<promise_type> _coroutine;

            awaitable_base(std::coroutine_handle<promise_type> coroutine) noexcept
                    : _coroutine(coroutine)
            {}

            bool await_ready() const noexcept {
                return !_coroutine || _coroutine.done();
            }


            bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
                _coroutine.resume();
                return _coroutine.promise().try_set_continuation(awaitingCoroutine);
            }
        };

    public:

        task() noexcept : _coroutine{nullptr}
        { }

        explicit task(std::coroutine_handle<promise_type> coroutine)
            : _coroutine{coroutine}
        { }

        task(task&& t) noexcept
                : _coroutine{std::exchange(t._coroutine, nullptr)}
        { }

        task(const task&) = delete;
        task& operator=(const task&) = delete;

        ~task() {
            if (_coroutine) {
                // free resources used by this task
                _coroutine.destroy();
            }
        }

        task& operator=(task&& other) noexcept {
            if (std::addressof(other) != this) {
                if (_coroutine) {
                    _coroutine.destroy();
                }

                _coroutine = other._coroutine;
                other._coroutine = nullptr;
            }

            return *this;
        }

        bool is_ready() const noexcept {
            return !_coroutine || _coroutine.done();
        }

        auto operator co_await() const & noexcept {
            struct awaitable : awaitable_base {
                using awaitable_base::awaitable_base;

                decltype(auto) await_resume() {
                    if (!this->m_coroutine) {
                        throw broken_promise{};
                    }

                    return this->_coroutine.promise().result();
                }
            };

            return awaitable{ _coroutine };
        }

        auto operator co_await() const && noexcept {
            struct awaitable : awaitable_base {
                using awaitable_base::awaitable_base;

                decltype(auto) await_resume()
                {
                    if (!this->_coroutine) {
                        throw broken_promise{};
                    }

                    return std::move(this->_coroutine.promise()).result();
                }
            };

            return awaitable{ _coroutine };
        }

        auto when_ready() const noexcept {
            struct awaitable : awaitable_base {
                using awaitable_base::awaitable_base;
                void await_resume() const noexcept {}
            };

            return awaitable{ _coroutine };
        }

    private:
        std::coroutine_handle<promise_type> _coroutine{};
    };

    namespace detail {

        template<bool I, typename T>
        task<I, T> task_promise<I, T>::get_return_object() noexcept
        {
            return task<I, T>{ std::coroutine_handle<task_promise>::from_promise(*this) };
        }

        template<bool I>
        inline task<I, void> task_promise<I, void>::get_return_object() noexcept
        {
            return task<I, void>{ std::coroutine_handle<task_promise>::from_promise(*this) };
        }

        template<bool I, typename T>
        task<I, T&> task_promise<I, T&>::get_return_object() noexcept
        {
            return task<I, T&>{ std::coroutine_handle<task_promise>::from_promise(*this) };
        }
    }

    template<typename A>
    auto make_task(A awaitable)
        -> task<false, remove_rvalue_reference_t<typename awaitable_traits<A>::await_result_t>>
    {
        co_return co_await static_cast<A&&>(awaitable);
    }

    template<typename A>
    auto make_async(A awaitable)
        -> task<true, remove_rvalue_reference_t<typename awaitable_traits<A>::await_result_t>>
    {
        co_return co_await static_cast<A&&>(awaitable);
    }

    template <typename T = void>
    using AsyncTask = task<false, T>;
    using AsyncVoid = AsyncTask<>;

    template <typename T = void>
    using Task = task<true, T>;
    using Void = Task<>;
}