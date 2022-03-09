/**
 * Copyright 2017 Lewis Baker
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 *
 * @author Modified by Carter from https://github.com/xuanyi-fu/xynet/blob/master/include/xynet/coroutine/async_scope.h
 * @date 2022-01-13
 */

#pragma once


#include <suil/async/coroutine.hpp>

#include <atomic>
#include <cassert>
#include <exception>

namespace suil {
    namespace detail {

        template <typename F>
        struct scoped_lambda {
            scoped_lambda(F&& func) noexcept : _func{std::forward<F>(func)}
            {}

            scoped_lambda(scoped_lambda&& other) noexcept
                : _func{std::forward<F>(other._func)},
                  _cancelled{std::exchange(other._cancelled, true)}
            {}

            scoped_lambda& operator=(scoped_lambda&& other) noexcept {
                if (this == std::addressof(other)) {
                    _func = std::forward<F>(other._func);
                    _cancelled = std::exchange(other._cancelled, true);
                }
                return *this;
            }

            ~scoped_lambda() {
                if (!_cancelled) {
                    _func();
                }
            }

            scoped_lambda(const scoped_lambda&) = delete;
            scoped_lambda& operator=(const scoped_lambda&) = delete;

            void cancel() { _cancelled = true; }

            void call() {
                _cancelled = true;
                _func();
            }
        private:
            F _func{nullptr};
            bool _cancelled{false};
        };

        template <typename F, bool CoF>
        struct conditional_scoped_lambda {

            conditional_scoped_lambda(F&& func) noexcept
                    : _func{std::forward<F>(func)},
                      _uncaughtExceptionCount{std::uncaught_exceptions()}
            {}

            conditional_scoped_lambda(conditional_scoped_lambda&& other) noexcept(std::is_nothrow_move_constructible<F>::value)
                : _func{std::forward<F>(other._func)}
                , _uncaughtExceptionCount{std::exchange(other._uncaughtExceptionCount, 0)}
                , _cancelled{std::exchange(other._cancelled, true)}
            {
                other.cancel();
            }

            conditional_scoped_lambda& operator=(conditional_scoped_lambda&& other) noexcept(std::is_nothrow_move_constructible<F>::value)
            {
                if (this != std::addressof(other)) {
                    _func = std::forward<F>(other._func);
                    _uncaughtExceptionCount = std::exchange(other._uncaughtExceptionCount, 0);
                    _cancelled = std::exchange(other._cancelled, true);
                }
                return *this;
            }

            ~conditional_scoped_lambda() noexcept(CoF || noexcept(std::declval<F>()()))
            {
                if (!_cancelled && (is_unwinding_due_to_exception() == CoF)) {
                    _func();
                }
            }

            conditional_scoped_lambda(const conditional_scoped_lambda& other) = delete;
            conditional_scoped_lambda& operator=(const conditional_scoped_lambda& other) = delete;

            void cancel() noexcept { _cancelled = true; }

        private:
            bool is_unwinding_due_to_exception() const noexcept {
                return std::uncaught_exceptions() > _uncaughtExceptionCount;
            }

            F _func{nullptr};
            int _uncaughtExceptionCount{0};
            bool _cancelled{false};
        };
    }

    template <typename F>
    auto onScopeExit(F&& func) {
        return detail::scoped_lambda<F>{std::forward<F>(func)};
    }

    template <typename F>
    auto onScopeFailure(F&& func) {
        return detail::conditional_scoped_lambda<F, true>{std::forward<F>(func)};
    }

    template <typename F>
    auto onScopeSuccess(F&& func) {
        return detail::conditional_scoped_lambda<F, true>{std::forward<F>(func)};
    }

    class AsyncScope {
    public:

        AsyncScope() noexcept
            : _count(1u)
        {}

        ~AsyncScope()  {
            // scope must be co_awaited before it destructs.
            SUIL_ASSERT(_continuation);
        }

        template<typename Awaitable>
        void spawn(Awaitable&& awaitable)
        {
            [](AsyncScope* scope, std::decay_t<Awaitable> awaitable) -> oneway_task
            {
                scope->on_work_started();
                auto decrementOnCompletion = onScopeExit([scope] { scope->on_work_finished(); });
                co_await std::move(awaitable);
            }(this, std::forward<Awaitable>(awaitable));
        }

        [[nodiscard]] auto join() noexcept
        {
            class awaiter
            {
                AsyncScope* _scope{nullptr};
            public:
                awaiter(AsyncScope* scope) noexcept : _scope(scope) {}

                bool await_ready() noexcept
                {
                    return _scope->_count.load(std::memory_order_acquire) == 0;
                }

                bool await_suspend(std::coroutine_handle<> continuation) noexcept
                {
                    _scope->_continuation = continuation;
                    return _scope->_count.fetch_sub(1u, std::memory_order_acq_rel) > 1u;
                }

                void await_resume() noexcept
                {}
            };

            return awaiter{ this };
        }

    private:

        void on_work_finished() noexcept {
            if (_count.fetch_sub(1u, std::memory_order_acq_rel) == 1) {
                _continuation.resume();
            }
        }

        void on_work_started() noexcept {
            assert(_count.load(std::memory_order_relaxed) != 0);
            _count.fetch_add(1, std::memory_order_relaxed);
        }

        struct oneway_task {

            struct promise_type {

                std::suspend_never initial_suspend() noexcept { return {}; }
                std::suspend_never final_suspend() noexcept { return {}; }
                void unhandled_exception() { std::terminate(); }
                oneway_task get_return_object() { return {}; }
                void return_void() {}
            };
        };

        std::atomic<size_t> _count{0};
        std::coroutine_handle<> _continuation{nullptr};
    };

}