//
// Created by Mpho Mbotho on 2020-12-30.
//

#include "suil/base/notify.hpp"
#include "suil/base/c/args.h"
using suil::String;
using suil::fs::Watcher;
using suil::fs::Event;
using suil::fs::Events;

int main(int argc, char *argv[])
{
    auto watcher = Watcher::create();
    auto wd = watcher->watch("/tmp/test", Events::Created|Events::Accessed);
    if (wd == -1) {
        return EXIT_FAILURE;
    }

    (*watcher)[wd] += [](const Event& ev) {
        sdebug("file /tmp/test/" PRIs " %s: {event %u, cookie %u, isDir %d, isUnmount %d}",
               _PRIs(ev.name), Events::str(ev.event), ev.event, ev.cookie, ev.isDir, ev.isUnMount);
        return true;
    };
    msleep(suil::Deadline{10000});
    return EXIT_SUCCESS;
}