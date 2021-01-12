//
// Created by Mpho Mbotho on 2020-12-30.
//

#include "suil/base/notify.hpp"
#include "suil/base/thread.hpp"

using suil::String;
using suil::fs::Watcher;
using suil::fs::Event;
using suil::fs::Events;

struct DummyWork {
    DummyWork(int64 backoff, int index)
        : backoff{backoff},
          index{index}
    {}

    int64 backoff{1000};
    int index{0};
    String message{"Hello World"};
};

class DummyWorker : public suil::ThreadExecutor<DummyWork> {
public:
    using suil::ThreadExecutor<DummyWork>::ThreadExecutor;
    void executeWork(Work &work) override {
        strace("work %d waiting %lld", work.index, work.backoff);
        msleep(suil::Deadline{work.backoff});
        if (work.index%50 == 0)
            sinfo("{%u} %d says " PRIs, mtid(), work.index, _PRIs(work.message));
    }
};

int main(int argc, char *argv[])
{
    auto watcher = Watcher::create();
    auto wd = watcher->watch("/tmp/test", Events::Created|Events::Accessed);
    if (wd == -1) {
        return EXIT_FAILURE;
    }
    srand(time(nullptr));
    suil::ThreadPool<DummyWork> pool("dummy");
    pool.setNumberOfWorkers(24);
    pool.setWorkerBackoffWaterMarks(250, 100);
    pool.template start<DummyWorker>();
    for (int i = 0; i < 1000; i++) {
        pool.emplace(std::rand()%1500, i);
    }

    //
    (*watcher)[wd] += [](const Event& ev) {
        sdebug("file /tmp/test/" PRIs " %s: {event %u, cookie %u, isDir %d, isUnmount %d}",
               _PRIs(ev.name), Events::str(ev.event), ev.event, ev.cookie, ev.isDir, ev.isUnMount);
        return true;
    };
    msleep(suil::Deadline{10000});
    return EXIT_SUCCESS;
}