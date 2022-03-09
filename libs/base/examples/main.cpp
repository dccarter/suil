//
// Created by Mpho Mbotho on 2020-12-30.
//

#include "suil/base/notify.hpp"

#include <suil/async/poll.hpp>
#include <suil/async/sync.hpp>
#include <suil/base/logging.hpp>

using suil::String;
using suil::fs::Watcher;
using suil::fs::Event;
using suil::fs::Events;
using namespace suil;

AsyncVoid watch()
{
    auto watcher = Watcher::create();
    auto wd = co_await watcher->watch("/tmp/test", Events::Created|Events::Accessed);
    co_await ((*watcher)[wd] += [](const Event& ev) {
        sdebug("file /tmp/test/" PRIs " %s: {event %u, cookie %u, isDir %d, isUnmount %d}",
               _PRIs(ev.name), Events::str(ev.event), ev.event, ev.cookie, ev.isDir, ev.isUnMount);
        return true;
    });

    co_await delay(5s);

    watcher->unwatch(wd);

    co_await (*watcher);

    Poll::poke(suil::Poll::POKE_STOP);
}

int main(int argc, char *argv[])
{
    suil::setup(opt(verbose, 0));
    auto watcher = watch();
    Poll::loop(1000ms);
    return 0;
}