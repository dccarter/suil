/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-07
 */

#include "suil/async/fdwait.hpp"
#include "suil/async/poll.hpp"

#include <cerrno>
#include <fcntl.h>

namespace suil {

    FDWaitContext::FDWaitContext(int fd, FDW fdw, steady_clock::time_point deadline) noexcept
        : fd{fd},
          fdw{fdw},
          waitDeadline{deadline}
    {}

    FDWaitContext::FDWaitContext(FDWaitContext &&o) noexcept
        : fd{std::exchange(o.fd, -1)},
          fdw{std::exchange(o.fdw, FDW_ERR)},
          waitingHandle{std::exchange(o.waitingHandle, {})},
          waitDeadline{std::exchange(o.waitDeadline, {})},
          waitTimer{std::exchange(o.waitTimer, std::nullopt)}
    {}

    FDWaitContext& FDWaitContext::operator=(FDWaitContext &&o) noexcept
    {
        fd = std::exchange(o.fd, -1);
        fdw = std::exchange(o.fdw, FDW_ERR);
        waitDeadline = std::exchange(o.waitDeadline, {});
        waitingHandle = std::exchange(o.waitingHandle, {});
        waitTimer = std::exchange(o.waitTimer, std::nullopt);
        return *this;
    }

    FDWaitContext::~FDWaitContext() noexcept(false)
    {
        if (fd != -1) {
            fd = -1;
        }
    }

    bool FDWaitContext::ready() const noexcept
    {
        auto _fd = fd;
        if (fcntl(_fd, F_GETFL, 0) & O_NONBLOCK)
            return false;
        // this should by-pass the waiting game
        return true;
    }

    void FDWaitContext::suspend(std::coroutine_handle<void> h) noexcept(false)
    {
        waitingHandle = h.address();
        Poll::tryAdd(this);
    }

    FDW FDWaitContext::resume() noexcept {
        if (waitTimer) {
            Poll::remove(fd);
            waitTimer = std::nullopt;
            errno = ETIMEDOUT;
        }

        return fdw;
    }
}