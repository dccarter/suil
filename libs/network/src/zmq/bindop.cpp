//
// Created by Mpho Mbotho on 2020-12-11.
//

#include "suil/net/zmq/operators.hpp"
#include "suil/net/zmq/socket.hpp"

namespace suil::net::zmq {

    bool BindOperator::bind(const String& endpoint)
    {
        if (!sock()) {
            lwarn(&sock(), "BindOperator::bind(...) socket is invalid");
            return false;
        }

        if (0 != zmq_bind(sock().raw(), endpoint())) {
            lwarn(&sock(), "BindOperator::bind(...) zmq_bind failed: %s", zmq_strerror(zmq_errno()));
            return false;
        }

        return true;
    }

#if (ZMQ_VERSION_MAJOR > 3) || ((ZMQ_VERSION_MAJOR == 3) && (ZMQ_VERSION_MINOR >= 2))
    void BindOperator::unbind(const String& endpoint)
    {
        if (!sock()) {
            lwarn(&sock(), "BindOperator::unbind(...) socket is invalid");
            return;
        }

        if (0 != zmq_unbind(sock().raw(), endpoint())) {
            lwarn(&sock(), "BindOperator::unbind(...) zmq_unbind failed: %s", zmq_strerror(zmq_errno()));
        }
    }
#endif
}