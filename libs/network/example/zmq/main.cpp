//
// Created by Mpho Mbotho on 2020-12-12.
//

#include "suil/net/zmq/context.hpp"
#include "suil/net/zmq/patterns.hpp"
#include "suil/net/zmq/monitor.hpp"

#include <suil/base/buffer.hpp>
#include <suil/base/datetime.hpp>

#include <libmill/libmill.h>

#include <iostream>
#include <string_view>

namespace zmq = suil::net::zmq;

static void freeData(void *data, void *hint)
{
    sdebug("freeing zmq message data %p", data);
    ::free(data);
}

static void freeStatic(void *data, void *hint)
{
    sdebug("freeing zmq message static data %p", data);
}

static coroutine void startRep(zmq::Context& ctx)
{
    zmq::ReplySocket rep{ctx};
    rep.bind("tcp://*:5555");
    zmq::SocketMonitor sm(ctx);
    sm.start(rep, "inproc://rep-monitor", ZMQ_EVENT_ACCEPTED);
    sm.onEvent += [](int16 ev, int32 value, const suil::String& addr) {
        sdebug("event: %hu %u " PRIs, ev, value, _PRIs(addr));
    };

    static suil::String say{"Hello awesome client..."};
    zmq::Message msg;
    if (!rep.receive(msg)) {
        serror("receiving message failed abort REPLY");
        return;
    }
    msg.gather([](const suil::Data& d) {
        std::cout << std::string_view{reinterpret_cast<const char*>(d.data()), d.size()};
    });
    std::cout << std::endl;
    zmq::Message out;
    out.emplace(say.data(), say.size(), freeStatic);
    suil::Buffer ob;
    ob << "\nmessage received at " << suil::Datetime()();
    auto size = ob.size();
    out.emplace(ob.release(), size, freeData, nullptr);
    if (!rep.send(out)) {
        serror("sending message failed abort REPLY");
        return;
    }
    msleep(suil::Deadline{3000});
}

int main(int argc, char *argv[])
{
    zmq::Context context;
    go(startRep(context));
    msleep(suil::Deadline{1500});

    zmq::RequestSocket req{context};
    req.connect("tcp://127.0.0.1:5555");

    static suil::String say{"Hello awesome server"};
    {
        zmq::Message out;
        out.emplace(say.data(), say.size(), freeStatic);
        suil::Buffer ob;
        ob << "\nmessage sent at " << suil::Datetime()();
        auto size = ob.size();
        out.emplace(ob.release(), size, freeData, nullptr);
        if (!req.send(out)) {
            serror("sending message failed abort REQUEST");
            return EXIT_FAILURE;
        }
    }

    {
        zmq::Message msg;
        if (!req.receive(msg)) {
            serror("receiving message failed abort REQUEST");
            return EXIT_FAILURE;
        }
        msg.gather([](const suil::Data& d) {
            std::cout << std::string_view{reinterpret_cast<const char*>(d.data()), d.size()};
        });
        std::cout << std::endl;
    }
    msleep(suil::Deadline{5000});
    return EXIT_SUCCESS;
};