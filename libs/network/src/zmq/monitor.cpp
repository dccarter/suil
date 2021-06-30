//
// Created by Mpho Mbotho on 2020-12-12.
//

#include "suil/net/zmq/monitor.hpp"

#include <libmill/libmill.h>

namespace suil::net::zmq {

    SocketMonitor::SocketMonitor(Context& ctx)
        : _sock(ctx)
    {}

    SocketMonitor::~SocketMonitor() noexcept
    {
        Ego.stop();
    }

    bool SocketMonitor::start(MonitorOperator& sock, const String& me, int events)
    {
        if (!sock.monitor(me, events)) {
            return false;
        }

        if (!_sock.connect(me)) {
            sock.unMonitor();
            return false;
        }
        _connected = true;

        go(waitForEvents(Ego));

        return true;
    }

    void SocketMonitor::waitForEvents(SocketMonitor& S) {
        ldebug(&S, "Starting wait for events %s", S._sock.id()());
        S._waiting = true;

        while (S._connected and S._sock) {
            Message msg;
            if (!S._sock.receive(msg)) {
                if (S._connected) {
                    ldebug(&S, "%s: waiting for events failed, aborting", S._sock.id()());
                }
                break;
            }
            if (msg.empty() or msg[0].size() < sizeof(Event)) {
                ldebug(&S, "%s: received an empty/invalid event", S._sock.id()());
                continue;
            }
            auto &frame = msg[0];
            Event event{0, 0};
            String addr;
            auto data = static_cast<const char *>(frame.data());
#if ZMQ_VERSION_MAJOR >= 4
            memcpy(&event.ev, data, sizeof(int16));
            data += sizeof(int16);
            memcpy(&event.value, (data + sizeof(int16)), sizeof(int32));
#else
            event = *static_cast<zmq_event *>(data);
            data += sizeof(Event);
#endif

#ifdef ZMQ_EVENT_MONITOR_STOPPED
            if (event.ev == ZMQ_EVENT_MONITOR_STOPPED) {
                ldebug(&S, "%s received ZMQ_EVENT_MONITOR_STOPPED event", S._sock.id()());
                break;
            }
#endif

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(3, 3, 0)
            if (msg.size() > 1) {
                auto &f2 = msg[1];
                addr = String{static_cast<const char *>(f2.data()), f2.size(), false}.dup();
            }
#else
            addr = String{data, frame.size(), false}.dup();
#endif
            S.onEvent(event.ev, event.value, addr);
        }

        S._waiting = false;
        if (S._connected) {
            // Stop event monitoring
            S.stop();
        }

        S._waiterEvent.notifyOne();
    }

    void SocketMonitor::stop()
    {
        if (Ego._connected) {
            Ego._connected = false;
            // Close the monitoring socket
            Ego._sock.close();
        }

        if (Ego._waiting) {
            // Wait for the coroutine to exit
            Sync sync;
            Ego._waiterEvent.wait();
        }
    }
}