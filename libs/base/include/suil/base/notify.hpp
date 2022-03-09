//
// Created by Mpho Mbotho on 2020-12-30.
//

#ifndef SUIL_BASE_NOTIFY_HPP
#define SUIL_BASE_NOTIFY_HPP

#include <suil/base/logging.hpp>
#include <suil/base/signal.hpp>
#include <suil/base/string.hpp>

#include <suil/utils/exception.hpp>
#include <suil/async/event.hpp>
#include <suil/async/scope.hpp>

#include <map>
#include <sys/inotify.h>

namespace suil::fs {

    define_log_tag(FS_NOTIFY);

    struct Events {
        enum : uint32 {
            Accessed = IN_ACCESS,
            AttribsChanged = IN_ATTRIB,
            WriteClosed = IN_CLOSE_WRITE,
            NonWriteClosed = IN_CLOSE_NOWRITE,
            Created = IN_CREATE,
            Deleted = IN_DELETE,
            SelfDeleted = IN_DELETE_SELF,
            Modified = IN_MODIFY,
            SelfMoved = IN_MOVE_SELF,
            MovedFrom = IN_MOVED_FROM,
            MovedTo = IN_MOVED_TO,
            Opened = IN_OPEN,
            Moved = IN_MOVE,
            Closed = IN_CLOSE
        };
        static const char *str(uint32 e);
    };

    struct WatchMode  {
        enum : uint32  {
            DontFollow = IN_DONT_FOLLOW,
            ExcludeLinks = IN_EXCL_UNLINK,
            MaskAdd = IN_MASK_ADD,
            OneShot = IN_ONESHOT,
            DirOnly = IN_ONLYDIR,
            MaskCreate = IN_MASK_CREATE
        };
    };

    DECLARE_EXCEPTION(FileWatcherException);

    struct Event {
        uint32  event{0};
        uint32  cookie{0};
        String  name{};
        bool    isDir{false};
        bool    isUnMount{false};
    };

    class Watcher : LOGGER(FS_NOTIFY) {
    public:
        using UPtr = std::unique_ptr<Watcher>;
        using Notifier = Signal<bool(const Event&)>;

        static Watcher::UPtr create();

        task<int> watch(
                const String& path,
                uint32 events,
                Notifier::Func cb,
                uint32 mode = WatchMode::ExcludeLinks | WatchMode::MaskCreate);

        task<int> watch(
                const String& path,
                uint32 events,
                uint32 mode = WatchMode::ExcludeLinks | WatchMode::MaskCreate);

        void unwatch(int wd);

        void close();

        Notifier& operator[](int wd);

        Signal<void()> onEventQueueOverflow;

        auto operator co_await() { return _waiterScope.join(); }

        ~Watcher();

    private:
        Watcher(int fd);
        static AsyncVoid waitForEvents(Watcher& S);
        void handleEvents(const char* events, size_t len);
        DISABLE_COPY(Watcher);
        DISABLE_MOVE(Watcher);
        int _fd{INVALID_FD};
        using Signals = std::map<int, std::unique_ptr<Notifier>>;
        Signals _signals{};
        std::atomic_bool _watching{false};
        AsyncScope _waiterScope{};
    };
}
#endif //SUIL_BASE_NOTIFY_HPP
