//
// Created by Mpho Mbotho on 2020-12-11.
//

#include "suil/net/zmq/operators.hpp"
#include "suil/net/zmq/socket.hpp"

namespace suil::net::zmq {

    bool ConnectOperator::doConnect(const String& endpoint)
    {
        if (!sock()) {
            mConnected = 0;
            lwarn(&sock(), "SendOperator::doConnect(...) socket is invalid");
            return false;
        }

        if (0 != zmq_connect(sock().raw(), endpoint())) {
            lwarn(&sock(), "BindOperator::doConnect(...) zmq_connect failed: %s", zmq_strerror(zmq_errno()));
            return false;
        }
        mConnected++;

        return true;
    }

    void ConnectOperator::doDisconnect(const String& endpoint)
    {
        if (!sock()) {
            mConnected = 0;
            lwarn(&sock(), "SendOperator::doDisconnect(...) socket is invalid");
            return;
        }

        if (0 != zmq_disconnect(sock().raw(), endpoint())) {
            lwarn(&sock(), "BindOperator::doDisconnect(...) zmq_disconnect failed: %s", zmq_strerror(zmq_errno()));
            return;
        }
        mConnected--;
    }
}