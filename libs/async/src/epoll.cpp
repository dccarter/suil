/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-07
 */

#include "epoll.hpp"
#include "suil/async/task.hpp"

#include <memory>
#include <unistd.h>
#include <sys/eventfd.h>

namespace suil {

    __thread EPoll* sPoller{nullptr};

    Poll& Poll::instance()
    {
        if (unlikely(sPoller == nullptr)) {
            sPoller = new EPoll;
        }
        return *sPoller;
    }

    EPoll::EPoll() noexcept(false)
        : Poll(),
          _efd{epoll_create1(EPOLL_CLOEXEC)}
    {
        SXY_ASSERT(_efd != -1);
        _poke = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        SXY_ASSERT(_poke != -1);

        struct epoll_event ev{.events = EPOLLET | EPOLLIN, .data = {.fd = _poke}};
        int rc = epoll_ctl(_efd, EPOLL_CTL_ADD, _poke, &ev);
        SXY_ASSERT(rc != -1);
    }

    EPoll::~EPoll() noexcept(false)
    {
        if (_efd > 0) {
            ::close(_efd);
            _efd = -1;
        }

        if (_poke > 0) {
            ::close(_poke);
            _poke = -1;
        }
    }

    void EPoll::doAdd(FDWaitContext* req) noexcept(false)
    {
        SXY_ASSERT(req->fdw != FDW_ERR);

        int op = EPOLL_CTL_ADD, ec = 0;

    TRY_OP:
        struct epoll_event ev {
            .events = EPOLLONESHOT | EPOLLET | EPOLLHUP | (req->fdw == FDW_IN? EPOLLIN: EPOLLOUT),
            .data = {.ptr = req }
        };

        ec = epoll_ctl(_efd, op, req->fd, &ev);
        if (ec == 0) {
            if (req->waitDeadline != steady_clock::time_point{}) {
                req->waitTimer = addTimer(req->waitingHandle, req->waitDeadline);
                req->fdw = FDW_TIMEOUT;
            }
            return;
        }
        if (errno == EEXIST) {
            // already added, modify
            op = EPOLL_CTL_MOD;
            goto TRY_OP;
        }

        throw std::system_error{errno, std::system_category(), "epoll_ctl(ADD|MOD)"};
    }

    void EPoll::doRemove(std::uint64_t fd)
    {
        struct epoll_event ev{};
        auto ec = epoll_ctl(_efd, EPOLL_CTL_DEL, int(fd), &ev);
        if (ec != 0) {
            throw std::system_error{errno, std::system_category(), "epoll_ctl(DEL)"};
        }
    }

    std::ptrdiff_t EPoll::doWait(std::span<AsyncResp> &output, int64_t timeout) noexcept(false)
    {
        struct epoll_event events[output.size()];
        auto count = epoll_wait(_efd,
                                    events,
                                    int(output.size()),
                                    int(pollWaitTimeout(timeout)));

        if (count == -1 and errno != EINTR) {
            throw std::system_error{errno, std::system_category(), "epoll_wait()"};
        }

        int found{0};
        for (int i = 0; i < count; i++) {
            if (events[i].data.fd == _poke) {
                handlePokeEvent();
                continue;
            }

            FDW fdw{FDW_ERR};
            if (events[i].events & EPOLLIN) fdw = FDW_IN;
            if (events[i].events & EPOLLOUT) fdw = FDW_OUT;

            output[found++] = AsyncResp{.ptr = events[i].data.ptr, .res = fdw};
        }

        return found;
    }

    void EPoll::handlePokeEvent()
    {
        uint64_t val{0};
        if (read(_poke, &val, sizeof(val)) != -1) {
            switch (_pokeCmd) {
                case POKE_STOP: {
                    ::close(_poke);
                    ::close(_efd);
                    _poke = -1;
                    _efd = -1;
                    break;
                }
                case POKE_NOOP: break;
                default:
                    printf("received poke: %lu\n", val);
                    break;
            }
        }
    }

    void EPoll::doPoke(Poke cmd)
    {
        _pokeCmd = cmd;
        uint64_t val{10};
        if (write(_poke, &val, sizeof(val)) < 0)
            throw std::system_error{errno, std::system_category()};
    }
}