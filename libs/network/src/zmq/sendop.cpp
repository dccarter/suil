//
// Created by Mpho Mbotho on 2020-12-11.
//

#include "suil/net/zmq/socket.hpp"
#include "suil/net/zmq/operators.hpp"

#include <libmill/libmill.h>

namespace suil::net::zmq {

    bool SendOperator::sendFlags(const void* buf, size_t len, int flags, const Deadline& dd)
    {
        if (!sock()) {
            lwarn(&sock(), "SendOperator::sendFlags(...) socket is invalid");
            return false;
        }

        do {
            flags |= ZMQ_DONTWAIT;
            auto sent = zmq_send(sock().raw(), buf, len, flags);
            if (sent == -1) {
                if (zmq_errno() == EAGAIN) {
                    // would block...
                    int ev = fdwait(sock().fd(), FDW_OUT, dd);
                    if (ev&FDW_ERR) {
                        lwarn(&sock(), "SendOperator::sendFlags(...) fdwait() failed: %s", errno_s);
                        return false;
                    }
                    continue;
                }
                lwarn(&sock(), "SendOperator::sendFlags(...) zmq_send() failed: %s", zmq_strerror(zmq_errno()));
                return false;
            }
            else {
                // data sent
                ltrace(&sock(), "SendOperator::sendFlags(...) %d bytes sent", sent);
                break;
            }
        } while (true);

        return true;
    }

    bool SendOperator::sendFlags(const Message& msg, int flags, const Deadline& dd)
    {
        if (!sock()) {
            lwarn(&sock(), "SendOperator::sendFlags(...) socket is invalid");
            return false;
        }
        flags |= ZMQ_DONTWAIT;

        auto sendFrame = [this, &dd](const Frame& frame, int flags) {
            do {
                auto sent = zmq_msg_send(const_cast<zmq_msg_t *>(&frame._msg), sock().raw(), flags);
                if (sent == -1) {
                    if (zmq_errno() == EAGAIN) {
                        // would block...
                        int ev = fdwait(sock().fd(), FDW_OUT, dd);
                        if (ev&FDW_ERR) {
                            lwarn(&sock(), "SendOperator::sendFlags(...) fdwait() failed: %s", errno_s);
                            return false;
                        }
                        continue;
                    }
                    lwarn(&sock(), "SendOperator::sendFlags(...) zmq_send() failed: %s", zmq_strerror(zmq_errno()));
                    return false;
                }
                else {
                    // data sent
                    ltrace(&sock(), "SendOperator::sendFlags(...) %d bytes sent", sent);
                    break;
                }
            } while (true);

            return true;
        };

        for (auto& frame: msg) {
            auto f = flags;
            if (&frame != &msg._frames.back()) {
                f = flags | ZMQ_SNDMORE;
            }

            if (!sendFrame(frame, f)) {
                return false;
            }
        }

        return true;
    }
}