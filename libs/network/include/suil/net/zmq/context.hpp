//
// Created by Mpho Mbotho on 2020-12-08.
//

#ifndef SUIL_NETWORK_ZMQ_CONTEXT_HPP
#define SUIL_NETWORK_ZMQ_CONTEXT_HPP

#include <suil/utils/utils.hpp>

namespace suil::net::zmq {

    class Context {
    public:
        Context();
        MOVE_CTOR(Context) noexcept;
        MOVE_ASSIGN(Context) noexcept;

        DISABLE_COPY(Context);

        operator void* ();

        void * operator() () {
            return mHandle;
        }

        ~Context();

    private:
        void *mHandle{nullptr};
    };
}
#endif //SUIL_NETWORK_ZMQ_CONTEXT_HPP
