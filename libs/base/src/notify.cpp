//
// Created by Mpho Mbotho on 2020-12-30.
//

#include "suil/base/notify.hpp"

#include <suil/async/fdwait.hpp>

namespace suil::fs {

    Watcher::Watcher(int fd)
        : _fd{fd}
    {}

    Watcher::~Watcher()
    {
        close();
    }

    Watcher::UPtr Watcher::create()
    {
        auto fd = inotify_init1(IN_NONBLOCK);
        if (fd == -1) {
            serror("Watcher::create - inotify_init1 failed %s", errno_s);
            return nullptr;
        }
        return Watcher::UPtr{new Watcher(fd)};
    }

    task<int> Watcher::watch(const String& path, uint32 events, Notifier::Func cb, uint32 mode)
    {
        if (_fd == -1) {
            ierror("Watcher not initialized");
            co_return -1;
        }

        int wd = inotify_add_watch(_fd, path(), events|mode);
        if (wd == -1) {
            ierror("Watcher::watch(" PRIs ", %x) - inotify_add_watch(...) failed: %s",
                        _PRIs(path), events, errno_s);
            co_return wd;
        }
        if (_signals.find(wd) == _signals.end()) {
            _signals.emplace(wd, std::make_unique<Notifier>());
        }

        if (cb) {
            co_await (*(_signals[wd]) += cb);
        }

        if (!_watching) {
            // starting receiving file events
            _watching = true;
            _waiterScope.spawn(waitForEvents(Ego));
        }

        co_return wd;
    }

    task<int> Watcher::watch(const String& path, uint32 events, uint32 mode)
    {
        return watch(path, events, nullptr, mode);
    }

    void Watcher::close()
    {
        if (_fd != INVALID_FD) {
            for (auto& sig: _signals) {
                if (inotify_rm_watch(_fd, sig.first) < 0) {
                    iwarn("Watcher::~(%d) - inotify_rm_watch(...) failed: %s",
                          sig.first, errno_s);
                }
            }
            nonblocking(_fd, false);
            SUIL_ASSERT(::close(_fd) != -1);
            _fd = -1;
        }
        _signals.clear();
    }

    Watcher::Notifier& Watcher::operator[](int wd)
    {
        auto it = _signals.find(wd);
        if (it == _signals.end()) {
            throw IndexOutOfBounds("Watcher descriptor '", wd, "' not found");
        }
        return *it->second;
    }

    void Watcher::unwatch(int wd)
    {
        // erase watcher signal
        _signals.erase(wd);
        if (_fd != -1) {
            // remove watch descriptor from watch list
            if (inotify_rm_watch(_fd, wd) < 0) {
                iwarn("Watcher::unwatch(%d) - inotify_rm_watch(...) failed: %s", wd, errno_s);
            }
        }
    }

    AsyncVoid Watcher::waitForEvents(Watcher& S)
    {
        ltrace(&S, "Watcher::waitForEvent(%d) start waiting for events", S._fd);
        do {
            char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
            auto nread = ::read(S._fd, buf, sizeof(buf));
            if (nread < 0) {
                if (errno != EAGAIN) {
                    // reading failed
                    lwarn(&S, "reading file events failed: %s", errno_s);
                    break;
                }
                // reading would block, wait for events
                waitForNotifications:
                auto ev = co_await fdwait(S._fd, FDW_IN, after(2s));
                if (ev == FDW_TIMEOUT && !S._signals.empty()) goto waitForNotifications;

                if (ev != FDW_IN) {
                    // wait for events failed
                    ltrace(&S, "waiting for file events failed: %s", errno_s);
                    break;
                }
                continue;
            };
            S.handleEvents(buf, nread);
        } while (!S._signals.empty());

        ltrace(&S, "Watcher::waitForEvent(%d) done waiting for events", S._fd);
        S._watching = false;
    }

    void Watcher::handleEvents(const char* events, size_t len)
    {
        auto ev = reinterpret_cast<const inotify_event *>(events);
        for (auto i = 0u; i < len; i += (sizeof(inotify_event) + ev->len)) {
            ev = reinterpret_cast<const inotify_event *>(&events[i]);
            if ((ev->mask & IN_IGNORED) == IN_IGNORED) {
                // Un-interesting
                continue;
            }

            if ((ev->mask & IN_Q_OVERFLOW) == IN_Q_OVERFLOW) {
                // invoke the overflow handler
                if (!onEventQueueOverflow) {
                    throw FileWatcherException("Event queue overflowed");
                }

                // invoke overflow handler
                _waiterScope.spawn(onEventQueueOverflow());
            }

            auto it = _signals.find(ev->wd);
            if (it == _signals.end()) {
                // no signal handler found, remove
                unwatch(ev->wd);
            }

            // Spawn handler
            auto& fire = *it->second;
            _waiterScope.spawn(fire({
                ev->mask & IN_ALL_EVENTS,
                ev->cookie,
                String{ev->name, ev->len, false},
                (ev->mask & IN_ISDIR) == IN_ISDIR,
                (ev->mask & IN_UNMOUNT) == IN_UNMOUNT
            }));
        }
    }

    const char* Events::str(uint32 e)
    {
        switch (e) {
            case Accessed:
                return "IN_ACCESS";
            case AttribsChanged:
                return "IN_ATTRIB";
            case WriteClosed:
                return "IN_CLOSE_WRITE";
            case NonWriteClosed:
                return "IN_CLOSE_NOWRITE";
            case Created:
                return "IN_CREATE";
            case Deleted:
                return "IN_DELETE";
            case SelfDeleted:
                return "IN_DELETE_SELF";
            case Modified:
                return "IN_MODIFY";
            case SelfMoved:
                return "IN_MOVE_SELF";
            case MovedFrom:
                return "IN_MOVED_FROM";
            case MovedTo:
                return "IN_MOVED_TO";
            case Opened:
                return "IN_OPEN";
            case Moved:
                return "IN_MOVE";
            case Closed:
                return "IN_CLOSE";
            default:
                return "UNKNOWN";
        }
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>
#endif