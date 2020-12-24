//
// Created by Mpho Mbotho on 2020-12-08.
//

#include "suil/net/zmq/context.hpp"

#include <zmq.h>

namespace suil::net::zmq {

    Context::Context()
        : mHandle{zmq_ctx_new()}
    {}

    Context::~Context()
    {
        if (mHandle) {
            zmq_ctx_destroy(mHandle);
            mHandle = nullptr;
        }
    }

    Context::Context(Context&& other) noexcept
        : mHandle{other.mHandle}
    {}

    Context& Context::operator=(Context&& other) noexcept
    {
        mHandle = other.mHandle;
        other.mHandle = nullptr;
        return Ego;
    }

    Context::operator void*()
    {
        return mHandle;
    }
}