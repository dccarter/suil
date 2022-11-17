/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-11-05
 */

#pragma once

#include <suil/base/buffer.hpp>
#include <suil/base/channel.hpp>
#include <suil/base/logging.hpp>
#include <suil/base/sio.hpp>

namespace suil {

    extern const uint8& spid;

    struct LockData {
        volatile uint64 serving{0};
        volatile uint64 next{0};
        uint8    on;
        uint32   id;
    } __attribute((packed));

    struct Lock {
        static void reset(LockData& lk, uint32 id) {
            lk.serving = 0;
            lk.next  = 0;
            lk.on    = 0x0A;
            lk.id    = id;
        }

        static void cancel(LockData& lk) {
            lk.on = 0x00;
        }

        Lock(LockData& lk)
            : lk(lk)
        {
            spinLock(lk);

            strace("{%ld} lock-%u acquired by suil-%hhu",
                   mnow(), lk.id, spid);
        }

        ~Lock() {
            unlock(lk);
            strace("{%ld} lock-%d released by suil-%hhu",
                   mnow(), lk.id, spid);
        }

        static bool spinLock(LockData& l, int64_t tout = -1) {
            bool status{true};
            /* Request lock ticket */
            int ticket = __sync_fetch_and_add(&l.next, 1);

            strace("{%ld} lock-%d Request (ticket %d,  next %d serving %d)",
                   mnow(), l.id, ticket, l.next, l.serving);
            if (tout > 0) {
                /* compute the time at which we should giveup */
                tout += mnow();
            }

            while (l.on && !__sync_bool_compare_and_swap(&l.serving, ticket, ticket))
            {
                if (tout > 0 && tout < mnow()) {
                    status = false;
                    break;
                }

                /*yield the CPU giving other tasks a chance to start*/
                yield();
            }

            strace("{%ld} lock %d issued (ticket %d,  next %d serving %d)",
                   mnow(), l.id, ticket, l.next, l.serving);

            return status;
        }

        static inline void unlock(LockData& l) {
            // serve the next waiting
            (void) __sync_fetch_and_add(&l.serving, 1);
        }
        LockData& lk;
    };

    define_log_tag(WORKER);

    struct IPCMessageHeader {
        uint8_t     id;
        uint8_t     src;
        size_t      len;
    } __attribute((packed));

    enum SystemMessage : uint8_t {
        MSG_SYS_PING = 0,
#define SYS_PING                0
        MSG_GET_RESPONSE,
#define GET_RESPONSE            1
        MSG_GET_STATS,
#define GET_STATS               2
        MSG_GET_MEMORY_INFO,
#define GET_MEMORY_INFO         3
        MSG_INITIALIZER_ENABLE,
#define INITIALIZER_ENABLE      4
        MSG_ENABLE_ROUTE,
#define ENABLE_ROUTE            5
        MSG_DISABLE_ROUTE,
#define DISABLE_ROUTE           6
        MSG_SYSTEM = 64
#define SYSTEM                  64
    };

    using IPCGetHandle = Channel<uint8, 16>;

    struct IPCGetPayload {
        chan         handle;
        int64        deadline;
        size_t       size;
        uint8        data[0];
    } __attribute((packed));

    using IPCGetHandler = std::function<void(void *, uint8)>;

    #define NWORKERS_SYSTEM (0xff)

#define IPC_MSG(msg)  (suil::SystemMessage::MSG_SYSTEM + msg)

    using MessageHandler = std::function<bool(uint8, uint8*, size_t, bool)>;
    using Work = std::function<int(void)>;
    using PostSpawnFunc = std::function<int(uint8)>;
    using CleanUpHandler = std::function<void(void)>;

    namespace ipc {

        struct GetReponseData {
            void *data{nullptr};
            size_t size{0};
            void release();
        };

        void initialize(int n);

        template <typename ...Opts>
        inline void init(Opts... opts) {
            struct Args {
                uint8 nworkers{NWORKERS_SYSTEM};
            } args;
            applyConfig(args, std::forward<Opts>(opts)...);

            return initialize(args.nworkers);
        }

        int spawn(Work, PostSpawnFunc ps = nullptr, PostSpawnFunc pps = nullptr);

        ssize_t send(uint8 dst, uint8 msg, const void *data, size_t len);

        static inline ssize_t send(uint8 dst, uint8 msg, const char *str) {
            return send(dst, msg, str, strlen(str));
        }

        static inline ssize_t send(uint8 dst, uint8 msg, const Buffer& b) {
            return send(dst, msg, b.data(), b.size());
        }

        static inline ssize_t send(uint8 dst, uint8 msg, const String& str) {
            return send(dst, msg, str.data(), str.size());
        }

        static inline ssize_t send(uint8 dst, uint8 msg, strview& sv) {
            return send(dst, msg, sv.data(), sv.size());
        }

        void broadcast(uint8 msg, const void *data, size_t len);

        uint8 broadcast(Channel<int>& ch, uint8 msg, const void *data, size_t len);

        inline void broadcast(uint8 msg) {
            broadcast(msg, nullptr, 0);
        }

        static inline void broadcast(uint8 msg, const char *str) {
            broadcast(msg, str, strlen(str));
        }

        static inline void broadcast(uint8 msg, const Buffer& b) {
            broadcast(msg, b.data(), b.size());
        }

        static inline void broadcast(uint8 msg, const String& str) {
            broadcast(msg, str.data(), str.size());
        }

        static inline void broadcast(uint8 msg, const strview& sv) {
            broadcast(msg, sv.data(), sv.size());
        }

        void registerHandler(uint8 msg, MessageHandler handler);

        void unRegisterHandler(uint8);

        void registerCleaner(CleanUpHandler handler);

        bool spinLock(const uint8 idx, int64 tout = -1);

        void spinUnlock(const uint8 idx);

        GetReponseData get(uint8 msg, uint8 w, int64 tout = -1);

        std::vector<GetReponseData> gather(uint8 msg, int64 tout = -1);

        void sendGetResponse(const void *token, uint8 to, const void *data, size_t len);

        void registerGetHandler(uint8 msg, IPCGetHandler handler);

        inline void release(GetReponseData data) { data.release(); }

        inline void release(std::vector<GetReponseData>& data) {
            for( auto d: data)
                d.release();
            data.clear();
        }
    };

    enum {
        SHM_ACCEPT_LOCK = 0,
        SHM_GET_LOCK,
        SHM_GATHER_LOCK
    };
}