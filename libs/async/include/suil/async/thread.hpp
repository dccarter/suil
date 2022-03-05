/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author modified by Carter from https://github.com/luncliff/coroutine/blob/master/interface/coroutine/pthread.h
 * @date 2022-01-08
 */

#pragma once

#include <suil/async/coroutine.hpp>

#include <system_error>
#include <pthread.h>

namespace suil {

    class ThreadJoiner final {
    public:
        struct promise_type;

        class ThreadSpawn final: public std::suspend_always {
            friend class promise_type;
            static void *resumeOnThread(void *coro);

        public:
            void await_suspend(std::coroutine_handle<void> task) noexcept(false) {
                if (auto ec = pthread_create(_tid, _attr, resumeOnThread, task.address())) {
                    throw std::system_error{errno, std::system_category()};
                }
            }

        private:
            explicit ThreadSpawn(pthread_t* tid, const pthread_attr_t *attr)
                : _tid{tid}, _attr{attr}
            {}

            pthread_t* const _tid;
            const pthread_attr_t* const _attr{nullptr};
        };

        ThreadJoiner(const promise_type *p) : _promise{p}
        {}

        operator pthread_t() const noexcept;

        ~ThreadJoiner() noexcept(false);
    private:
        const promise_type* _promise{nullptr};
    };

    class ThreadJoiner::promise_type {
    public:
        auto initial_suspend() noexcept { return std::suspend_never{}; }

        auto final_suspend() noexcept {return std::suspend_always{}; }

        auto return_void() { }

        void unhandled_exception() {}

        auto await_transform(const pthread_attr_t* attr) {
            return ThreadSpawn{std::addressof(tid), attr};
        }

        auto get_return_object() noexcept {
            return ThreadJoiner{this};
        }

        pthread_t tid{};
    };
}
