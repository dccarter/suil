/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-07
 */

#pragma once

#include <version>
#include <chrono>

#if defined(__cpp_lib_coroutine)
#include <coroutine>
#else
#include <experimental/coroutine>
#endif

#include <suil/base/utils.hpp>


namespace std {
#if !defined(__cpp_lib_coroutine)
    using namespace std::experimental;
#endif
}

namespace suil {
    namespace detail {
        struct any
        {
            template<typename T>
            any(T&&) noexcept
            {}
        };

        template <typename T>
        struct is_coroutine_handle : std::false_type {};

        template <typename P>
        struct is_coroutine_handle<std::coroutine_handle<P>> : std::true_type {};

        template <typename T>
        struct is_valid_await_suspend_return_value :
                std::disjunction<std::is_void<T>, std::is_same<T, bool>, is_coroutine_handle<T>>
        {};

        template <typename T, typename = std::void_t<>>
        struct is_awaiter : std::false_type {};

        template <typename T>
        struct is_awaiter<T, std::void_t<
                decltype(std::declval<T>().await_ready()),
                decltype(std::declval<T>().await_suspend(std::declval<std::coroutine_handle<>>())),
                decltype(std::declval<T>().await_resume())>> :
            std::conjunction<
                std::is_constructible<bool, decltype(std::declval<T>().await_ready())>,
                detail::is_valid_await_suspend_return_value<
                    decltype(std::declval<T>().await_suspend(std::declval<std::coroutine_handle<>>()))>>
        {};

        template <typename T>
        auto get_awaiter_impl(T&& value, int) noexcept(noexcept(static_cast<T&&>(value).operator co_await()))
            -> decltype(static_cast<T&&>(value).operator co_await())
        {
            return static_cast<T&&>(value).operator co_await();
        }

        template<typename T>
        auto get_awaiter_impl(T&& value, long) noexcept(noexcept(operator co_await(static_cast<T&&>(value))))
            -> decltype(operator co_await(static_cast<T&&>(value)))
        {
            return operator co_await(static_cast<T&&>(value));
        }

        template<typename T,
                std::enable_if_t<detail::is_awaiter<T&&>::value, int> = 0>
        T&& get_awaiter_impl(T&& value, detail::any) noexcept
        {
            return static_cast<T&&>(value);
        }

        template<typename T>
        auto get_awaiter(T&& value) noexcept(noexcept(detail::get_awaiter_impl(static_cast<T&&>(value), 123)))
            -> decltype(detail::get_awaiter_impl(static_cast<T&&>(value), 123))
        {
            return detail::get_awaiter_impl(static_cast<T&&>(value), 123);
        }
    }

    template <typename T>
    struct awaitable_traits {
        using awaiter_t = decltype(detail::get_awaiter(std::declval<T>()));
        using await_result_t = decltype(std::declval<awaiter_t>().await_resume());
    };

    template<typename T, typename = std::void_t<>>
    struct is_awaitable : std::false_type {};

    template<typename T>
    struct is_awaitable<T, std::void_t<decltype(detail::get_awaiter(std::declval<T>()))>>
            : std::true_type
    {};

    template<typename T>
    constexpr bool is_awaitable_v = is_awaitable<T>::value;
}