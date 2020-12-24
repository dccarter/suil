//
// Created by Mpho Mbotho on 2020-12-11.
//

#include "suil/net/zmq/operators.hpp"
#include "suil/net/zmq/socket.hpp"

namespace suil::net::zmq {

#if (ZMQ_VERSION_MAJOR >= 4)
    bool MonitorOperator::monitor(const String& me, int events)
    {
        if (!sock()) {
            lwarn(&sock(), "MonitorOperator::monitor(...) socket is invalid");
            return false;
        }

        if (0 != zmq_socket_monitor(sock().raw(), me(), events)) {
            lwarn(&sock(), "MonitorOperator::monitor(...) zmq_socket_monitor failed: %s", zmq_strerror(zmq_errno()));
            return false;
        }

        return true;
    }

    void MonitorOperator::unMonitor()
    {
        if (!sock()) {
            lwarn(&sock(), "MonitorOperator::unMonitor(...) socket is invalid");
            return;
        }

        if (0 != zmq_socket_monitor(sock().raw(), NULL, 0)) {
            lwarn(&sock(), "MonitorOperator::unMonitor(...) zmq_socket_monitor failed: %s", zmq_strerror(zmq_errno()));
            return;
        }
    }
#endif
}