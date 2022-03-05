/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Modified by Carter from https://github.com/luncliff/coroutine/blob/master/interface/coroutine/yield.hpp
 * @date 2022-01-07
 *
 * // file: coroutine/yield.hpp
 * // author: github.com/luncliff (luncliff@gmail.com)
 * // brief: `enumerable` is simply a copy of `generator` in VC++
 * // copyright: CC BY 4.0
 */

#pragma once

#include "suil/async/coroutine.hpp"

namespace suil {

    template <typename T>
    struct Enumerable {
        struct promise_type;
        struct iterator;

        using value_type = T;
        using reference = value_type&;
        using pointer = value_type*;

        Enumerable() = default;
        Enumerable(std::coroutine_handle<promise_type> co) noexcept
                : _coro{co}
        {}

        Enumerable(const Enumerable&) = delete;
        Enumerable& operator=(const Enumerable&) = delete;

        Enumerable(Enumerable&& o) noexcept
                : _coro{std::exchange(o._coro, nullptr)}
        {}

        Enumerable& operator=(Enumerable& o) noexcept {
            _coro = std::exchange(o._coro, nullptr);
            return *this;
        }

        ~Enumerable() noexcept {
            if (_coro) {
                _coro.destroy();
                _coro = nullptr;
            }
        }

        iterator begin() noexcept(false) {
            if (_coro) {
                _coro.resume();
                if (_coro.done())
                    return iterator{nullptr};
            }
            return iterator{_coro};
        }

        iterator end() noexcept { return iterator{nullptr}; }

    private:
        std::coroutine_handle<promise_type> _coro{};
    };

    template <typename T>
    struct Enumerable<T>::promise_type final {
        friend struct iterator;
        friend struct Enumerable;

        Enumerable get_return_object() noexcept {
            return Enumerable{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        void unhandled_exception() noexcept(false) { throw; }

        auto yield_value(reference ref) noexcept {
            current = std::addressof(ref);
            return std::suspend_always{};
        }

        auto yield_value(value_type&& v) noexcept { return yield_value(v); }

        void return_void() noexcept {
            current = nullptr;
        }

    private:
        pointer current{nullptr};
    };

    template <typename T>
    struct Enumerable<T>::iterator final {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type  = T;
        using reference   = T&;
        using pointer     = T*;

    public:
        explicit iterator(std::nullptr_t) noexcept : _coro{nullptr}
        {}
        explicit iterator(std::coroutine_handle<promise_type> handle): _coro{handle}
        {}

    public:
        iterator& operator++(int) = delete;

        iterator& operator++() noexcept(false) {
            _coro.resume();
            if (_coro.done()) _coro = nullptr;
            return *this;
        }

        pointer operator->() noexcept {
            auto ptr = _coro.promise().current;
            return ptr;
        }

        reference operator*() noexcept {
            return *(this->operator->());
        }

        bool operator==(const iterator& rhs) const noexcept { return this->_coro == rhs._coro; }
        bool operator!=(const iterator& rhs) const noexcept { return this->_coro != rhs._coro; }

    private:
        std::coroutine_handle<promise_type> _coro{};
    };
}
