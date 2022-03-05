/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-01-08
 */

#pragma once

#include <suil/async/fdwait.hpp>
#include <suil/async/task.hpp>
#include <suil/base/utils.hpp>

#include <span>

namespace suil {

    namespace fdops {
        void nonblocking(int fd, bool on);
        auto read(int fd, std::span<char> buf, std::chrono::milliseconds timeout = DELAY_INF) -> Task<int>;
        auto write(int fd, const std::span<const char> &buf, std::chrono::milliseconds timeout = DELAY_INF) -> Task<int>;
    }
}