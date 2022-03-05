/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author modified by Carter from https://github.com/luncliff/coroutine/blob/master/interface/coroutine/linux.h
 * @date 2022-01-07
 *
 * author: github.com/luncliff (luncliff@gmail.com)
 */

#pragma once
#if !(defined(__linux__))
#error "Expecting Linux platform for this file"
#endif

#include "suil/async/poll.hpp"

#include <sys/epoll.h>

namespace suil {

    class EPoll final : public Poll {
    public:
        EPoll() noexcept(false);
        ~EPoll() noexcept(false);

        EPoll(const EPoll&) = delete;
        EPoll& operator=(const EPoll&) = delete;
        EPoll(EPoll&&) = delete;
        EPoll& operator=(EPoll&&) = delete;

    protected:
        bool isArmed() const override { return _efd != -1; }
        void doPoke(Poke cmd) override;
        void doAdd(FDWaitContext* req) noexcept(false) override;
        void doRemove(std::uint64_t fd) override;
        std::ptrdiff_t doWait(
                std::span<AsyncResp> &output, int64_t timeout) noexcept(false) override;

    private:
        void handlePokeEvent();
        
        int _efd{-1};
        int _poke{-1};
        Poke _pokeCmd{Poke::POKE_NOOP};
    };
}