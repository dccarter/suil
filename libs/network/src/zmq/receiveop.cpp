//
// Created by Mpho Mbotho on 2020-12-11.
//

#include "suil/net/zmq/socket.hpp"
#include "suil/net/zmq/operators.hpp"

#include <libmill/libmill.h>

namespace suil::net::zmq {

    bool ReceiveOperator::receiveFlags(Message& msg, int flags, const Deadline& dd)
    {
        if (!sock()) {
            lwarn(&sock(), "ReceiveOperator::receiveFlags(Message...) socket is invalid");
            return false;
        }
        flags |= ZMQ_DONTWAIT;

        auto receiveFrame = [this, &dd, flags](Frame& frame) {
            do {
#if (ZMQ_VERSION_MAJOR == 2)
                auto res = zmq_recv(sock().raw(), &frame.raw(), flags);
#elif (ZMQ_VERSION_MAJOR < 3) || ((ZMQ_VERSION_MAJOR == 3) && (ZMQ_VERSION_MINOR < 2))
                auto res = zmq_recvmsg(sock().raw(), &frame.raw(), flags);
#else
                auto res = zmq_msg_recv(&frame.raw(), sock().raw(), flags);
#endif
                if (res == -1) {
                    if (zmq_errno() == EAGAIN) {
                        // nothing to ready yet, wait
                        auto ev = fdwait(sock().fd(), FDW_IN, dd);
                        if (ev & FDW_ERR) {
                            // was not aborted
                            ldebug(&sock(), "ReceiveOperator::receiveFlags(Message...) fdwait error: %s", errno_s);
                            return false;
                        }
                        continue;
                    }
                    lwarn(&sock(), "ReceiveOperator::receiveFlags(Message...) zmq_msg_recv failed: %s",
                                zmq_strerror(zmq_errno()));
                    return false;
                }
                else {
                    itrace("ReceiveOperator::receiveFlags(Message...) received frame %zu bytes", frame.size());
                    break;
                }
            } while (true);

            return true;
        };

        int needsMore{0};
        do {
            Frame frame{};
            if (!frame) {
                throw ZmqInternalError("ReceiveOperator::receiveFlags(Message...) - allocating new ZMQ message frame failed");
            }

            if (!receiveFrame(frame)) {
                // abort if receiving any of the frames fails
                return false;
            }
            msg.add(std::move(frame));
            if (!Option::getReceiveMore(sock(), needsMore)) {
                // getting socket option failed
                return false;
            }
        } while (needsMore != 0);

        return true;
    }

    bool ReceiveOperator::receiveFlags(void* buf, size_t& len, int flags, const Deadline& dd)
    {
        if (!sock()) {
            lwarn(&sock(), "ReceiveOperator::receiveFlags(buf...) socket is invalid");
            return false;
        }
        flags |= ZMQ_DONTWAIT;

        do {
            auto res = zmq_recv(sock().raw(), buf, len, flags);
            if (res == -1) {
                if (zmq_errno() == EAGAIN) {
                    auto ev = fdwait(sock().fd(), FDW_IN, dd);
                    if (ev & FDW_ERR) {
                        ldebug(&sock(), "ReceiveOperator::receiveFlags(buf...) fdwait error: %s", errno_s);
                        return false;
                    }
                    continue;
                }
                lwarn(&sock(), "ReceiveOperator::receiveFlags(buf...) zmq_recv failed: %s",
                      zmq_strerror(zmq_errno()));
                return false;
            }
            else {
                itrace("ReceiveOperator::receiveFlags(Message...) received frame %zu bytes", frame.size());
                len = res;
                break;
            }
        } while (true);

        return true;
    }

}