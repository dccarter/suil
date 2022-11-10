/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-11-08
 */

#include <csignal>
#include <utility>
#include <sys/shm.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <fcntl.h>

#include "suil/base/ipc.hpp"

#ifndef WORKER_DATA_SIZE
#define WORKER_DATA_SIZE    256
#endif

#ifndef WORKER_SHM_LOCKS
#define WORKER_SHM_LOCKS    64
#endif

/*
 * On some systems WAIT_ANY is not defined
 * */
#ifndef WAIT_ANY
#define WAIT_ANY (-1)
#endif

#ifndef IPC_MAX_NUMBER_OF_MESSAGES
#define IPC_MAX_NUMBER_OF_MESSAGES uint8((1<<8)-1)
#endif

namespace suil {

    uint8 workerProcessId = 0;
    const uint8& spid = workerProcessId;
    bool workerStarted = false;
    const bool& started = workerStarted;

#define SPID_PARENT 0

    struct Worker {
        int         fd[2];
        pid_t       pid;
        LockData        lock;
        uint64      mask[4];
        bool        active;
        uint8       cpu;
        uint8       id;
        uint8       data[WORKER_DATA_SIZE];
    } __attribute((packed));

    struct IPCInfo {
        uint8_t     nworkers;
        uint8_t     nactive;
        LockData    locks[WORKER_SHM_LOCKS];
        Worker      workers[0];
    } __attribute((packed));

    static IPCInfo   *IPC = nullptr;
    static int shmIpcId;

    struct WorkerLog : public LOGGER(WORKER) { WorkerLog() noexcept = default; } workerLog;
    static decltype(workerLog)* WLOG = &workerLog;

    static  MessageHandler ipcHandlers[256] = {nullptr};

    static std::vector<CleanUpHandler> cleanupHandlers{};
    using InflightGetData = std::vector<Data>;
    static std::unordered_map<chan, InflightGetData> inflightGets{};

    static inline bool hasMessageHandler(uint8_t w, uint8_t m) {
        Worker& wrk   = IPC->workers[w];
        if(w <= IPC->nworkers && wrk.active) {
            auto idx = (uint8) floor(m / 64);
            uint64 mask = (uint64_t) 1 << (m % 64);
            uint64 val = __sync_add_and_fetch(&wrk.mask[idx], 0);
            return (val & mask) == mask;
        }

        return false;
    }

    static inline void setMessageHandler(uint8_t w, uint8_t m, bool en) {
        if (w <= IPC->nworkers) {
            Worker &wrk = IPC->workers[w];
            auto idx = (uint8_t) floor(m / 64);
            uint64_t mask = (uint64_t) 1 << (m % 64);
            if (en) {
                __sync_fetch_and_or(&wrk.mask[idx], mask);
            } else {
                __sync_fetch_and_and(&wrk.mask[idx], ~mask);
            }
        }
    }

    static volatile sig_atomic_t sigReceived;
    static int signalNotif[2] = {-1, -1};
    static void __on_signal(int sig) {
        if (spid != SPID_PARENT || IPC->nworkers == 0) {
            if (sigReceived == 0) {
                sigReceived = sig;
                ldebug(WLOG, "worker worker/%hhu cleanup %d", spid, sig);
                for (auto& cleaner : cleanupHandlers) {
                    /* call cleaners */
                    cleaner();
                }
                cleanupHandlers.clear();
            }
        }
        else if (IPC->nworkers != 0 && spid == SPID_PARENT && sigReceived != sig) {
            ltrace(WLOG, "parent received signal %d", sig);
            sigReceived = sig;
            ::write(signalNotif[1], &sig, 1);
        }
    }

    namespace ipc {
        static coroutine void asyncReceive(Worker &wrk);

        static int receiveMessage(Worker &wrk, int64 dd);

        static void registerGetResponse();

        static int workerWait(bool last);

        static coroutine void invokeHandler(MessageHandler h, uint8 src, uint8 *data, size_t len, bool own);

        static int waitRead(int fd, int64 timeout = -1) {
            int64 tmp = timeout < 0? -1 : mnow() + timeout;
            int events = fdwait(fd, FDW_IN, tmp);
            if (events & FDW_ERR) {
                return -1;
            } else if (events & FDW_IN) {
                return 0;
            }

            return ETIMEDOUT;
        }

        static int waitWrite(int fd, int64 timeout = -1) {
            int64 tmp = timeout < 0? -1 : mnow() + timeout;
            int events = fdwait(fd, FDW_OUT, tmp);
            if (events&FDW_ERR) {
                return -1;
            } else if (events&FDW_OUT) {
                return 0;
            }

            return ETIMEDOUT;
        }

        void waitForSignal()
        {
            if (signalNotif[0] == -1)
                return;

            do {
                int ev = fdwait(signalNotif[0], FDW_IN | FDW_ERR, Deadline::Inf);
                if (ev & FDW_IN) {
                    // notification received
                    uint8 sig{0};
                    if (::read(signalNotif[0], &sig, sizeof(sig)) == sizeof(sig)) {
                        break;
                    }
                }
                ltrace(WLOG, "error while waiting for process to exit: %s", errno_s);
                break;

            } while (false);
        }

        static inline bool messageCheck(Worker& wrk, uint8_t msg) {
            return false;
        }

        static void init(Worker&  wrk) {
            // set the process id
            workerProcessId = wrk.id;
            wrk.pid = getpid();

            // set worker process name
            char name[16];
            snprintf(name, sizeof(name)-1, "worker/%hhu", spid);
            prctl(PR_SET_NAME, name);

            // set process affinity
            if (spid != SPID_PARENT) {
                cpu_set_t mask;
                CPU_ZERO(&mask);
                CPU_SET(wrk.cpu, &mask);
                sched_setaffinity(0, sizeof(mask), &mask);
                ldebug(WLOG, "worker/%hhu scheduled on cpu %hhu", spid, wrk.cpu);
            }

            if (spid == SPID_PARENT) {
                // configure signal notification
                if (pipe(signalNotif) == -1) {
                    // forking process failed
                    lcritical(WLOG, "error opening notification pipe: %s", errno_s);
                }

                // the read end of the pipe should be non-blocking
                suil::nonblocking(signalNotif[0]);
            }

            // Setup pipe's
            for (uint8 w = 0; w <= IPC->nworkers; w++) {
                Worker& tmp = IPC->workers[w];
                if (w == spid) {
                    // close the writing end for current process
                    close(tmp.fd[1]);
                    nonblocking(tmp.fd[0]);
                }
                else {
                    // close the reading ends for all the other processes
                    close(tmp.fd[0]);
                    nonblocking(tmp.fd[1]);
                }
            }

            signal(SIGHUP,  __on_signal);
            signal(SIGQUIT, __on_signal);
            signal(SIGTERM, __on_signal);
            signal(SIGINT,  __on_signal);
            signal(SIGKILL, __on_signal);
            signal(SIGPIPE, SIG_IGN);
            if (spid == SPID_PARENT) {
                signal(SIGCHLD, __on_signal);
            }

            wrk.active = true;

            if (spid != SPID_PARENT) {
                __sync_fetch_and_add(&IPC->nactive, 1);
                // start receiving messages
                go(asyncReceive(wrk));
            }

            workerStarted = true;
        }

        void initialize(int n)
        {
            int status = 0;

            if (IPC || spid != 0) {
                lcritical(WLOG, "system supports only 1 IPC group");
            }

            int quit = 0;
            uint8 cpu = 0;
            // spawn different process
            long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
            if (n == 0xff) 
                n = ncpus;
            if (n > ncpus)
                lwarn(WLOG, "number of workers more than number of CPU's");

            // create our worker's ipc
            size_t len = sizeof(Worker) * (n+1);
            shmIpcId =  shmget(IPC_PRIVATE, len, IPC_EXCL | IPC_CREAT | 0700);
            if (shmIpcId == -1)
                lcritical(WLOG, "shmget() error: %s", errno_s);

            void *shm = shmat(shmIpcId, nullptr, 0);
            if (shm == (void *) -1) {
                lerror(WLOG, "shmat() error: %s", errno_s);
                status = errno;
                goto ipc_dealloc;
            }

            IPC = (IPCInfo *) shm;

            // clear the attached memory
            memset(IPC, 0, len);
            // initialize accept lock
            for (uint8 i = 0; i < WORKER_SHM_LOCKS; i++)
                Lock::reset(IPC->locks[i], 256 + i);

            // initialize worker memory and pipe descriptors
            IPC->nworkers = n;
            IPC->nactive = 0;

            for (uint8 w = 0; w <= n; w++) {
                Worker& wrk = IPC->workers[w];
                Lock::reset(wrk.lock, w);
                wrk.id = w;
                wrk.cpu = cpu;

                if (pipe(wrk.fd) < 0) {
                    lerror(WLOG, "ipc - opening pipes for suil-%hhu failed: %s",
                           w, errno_s);
                    status = errno;
                    goto ipc_detach;
                }
                if (n)
                    cpu = (uint8) ((cpu +1) % ncpus);
            }
            registerGetResponse();

            return;

        ipc_detach:
            shmdt(shm);
            IPC = nullptr;

        ipc_dealloc:
            shmctl(shmIpcId, IPC_RMID, nullptr);
            if (status)
                lcritical(WLOG, "ipc::init failed");
        }

        int spawn(Work work, PostSpawnFunc ps, PostSpawnFunc pps) {
            if (IPC == NULL) {
                lcritical(WLOG, "IPC not initialized, call ipc::init() before spawning a worker");
            }

            int quit = 0, n = IPC->nworkers;
            int status = 0;

            // spawn worker process
            for (uint8 w = 1; w <= n; w++) {
                pid_t pid = mfork();
                if (pid < 0) {
                    lerror(WLOG, "spawning worker %hhu/%hhu failed: %s",
                           w, n, errno_s);
                    status = errno;
                    goto ipc_detach;
                }
                else if (pid == 0) {
                    prctl(PR_SET_PDEATHSIG, SIGHUP);
                    init(IPC->workers[w]);
                    break;
                }
            }

            sigReceived = 0;
            if (n == 0)
                init(IPC->workers[SPID_PARENT]);

            if (n == 0 || spid != SPID_PARENT) {
                lnotice(WLOG, "worker/%hhu started", spid);
                // if not parent handle work in continuous loop
                if (ps) {
                    // start post spawn delegate
                    try {
                        quit = ps(spid);
                    }
                    catch(...) {
                        lerror(WLOG, "unhandled exception in post spawn delegate");
                        quit  = 1;
                    }
                }

                while (sigReceived == 0 && !quit) {
                    try {
                        quit = work();
                    }
                    catch (const std::exception &ex) {
                        if (sigReceived == 0)
                            lerror(WLOG, "unhandled error in work: %s", ex.what());
                        break;
                    }
                }


                lnotice(WLOG, "worker/%hhu exit", spid, sigReceived);
                __sync_fetch_and_sub(&IPC->nactive, 1);

                // force worker to exit
                workerStarted = false;
                if (n != 0)
                    exit(quit);
            }
            else {
                Worker& wrk = IPC->workers[SPID_PARENT];
                init(wrk);
                lnotice(WLOG, "parent/%d started", wrk.pid);

                if (pps) {
                    // start post spawn delegate
                    try {
                        sigReceived = pps(spid);
                    }
                    catch(...) {
                        lerror(WLOG, "unhandled exception in parent post spawn delegate");
                        sigReceived  = EXIT_FAILURE;
                    }
                }

                // loop until a termination signal is received from all workers exited
                uint8_t done = 0;
                while (!quit) {
                    if (sigReceived) {
                        quit = sigReceived != SIGCHLD;
                        if (quit) {
                            for (uint8_t w = 1; w <= n; w++) {
                                // pass signal through to worker processes
                                Worker& tmp = IPC->workers[w];
                                if (tmp.active && kill(tmp.pid, SIGTERM) == -1) {
                                    ldebug(WLOG, "kill worker/%d failed: %s", tmp.pid, errno_s);
                                }
                            }

                            while (done < IPC->nworkers) {
                                // wait for workers to shut down
                                for (uint8_t w = 1; w <= n; w++) {
                                    // pass signal through to worker processes
                                    Worker &tmp = IPC->workers[w];
                                    if (tmp.active) {
                                        if (workerWait(true) == ECHILD) {
                                            break;
                                        }
                                        done++;
                                    }
                                }
                            }

                            continue;
                        }
                        else {
                            done++;
                        }
                        sigReceived = 0;
                    }

                    if (done >= IPC->nworkers || workerWait(false) == ECHILD) {
                        ldebug(WLOG, "all child process exited %hhu/%hhu",
                                     done, IPC->nactive);
                        quit = true;
                        continue;
                    }


                    waitForSignal();
                }
                lnotice(WLOG, "parent exiting (%d)", sigReceived);
                workerStarted = false;
                closepipe(signalNotif);
            }

        ipc_detach:
            shmdt(IPC);
            IPC = nullptr;

        ipc_dealloc:
            shmctl(shmIpcId, IPC_RMID, nullptr);
            if (status)
                lcritical(WLOG, "ipc::spawn failed");

            return 0;
        }

        ssize_t send(uint8 dst, uint8 msg, const void *data, size_t len) {
            ltrace(WLOG, "worker::send - dst %hhu, msg %hhu, data %p, len %lu",
                   dst, msg, data, len);
            errno = 0;

            if (spid == dst || dst > IPC->nworkers) {
                // prevent self destined messages
                lwarn(WLOG, "dst %hhu is an invalid send destination", dst);
                errno = EINVAL;
                return -1;
            }

            if (!hasMessageHandler(dst, msg)) {
                // worker does not handle the given message
                ltrace(WLOG, "dst %hhu does not handle message %02X", dst, msg);
                errno = ENOTSUP;
                return -1;
            }

            IPCMessageHeader hdr{};
            hdr.len = len;
            hdr.id = msg;
            hdr.src = spid;
            Worker &wrk = IPC->workers[dst];

            // acquire send lock of destination
            Lock l(wrk.lock);

            ltrace(WLOG, "sending header %02X to worker %hhu", msg, dst);
            do {
                int n = 0;
                if ((n = ::write(wrk.fd[1], &hdr, sizeof(hdr))) <= 0) {
                    int ws = 0;
                    if (errno == EAGAIN || errno == -EWOULDBLOCK) {
                        ws = waitWrite(wrk.fd[1]);
                        if (ws) {
                            lwarn(WLOG, "wait to sending message header to %hhu failed: %s",
                                  dst, errno_s);
                            return -1;
                        }
                        continue;
                    }

                    lwarn(WLOG, "sending message header to %hhu failed: %d %s",
                          dst, n, errno_s);
                    return -1;
                }
                break;
            } while (true);

            if (len == 0) {
                return 0;
            }

            ltrace(WLOG, "sending data of size %lu to worker %hhu", len, dst);
            ssize_t nsent = 0, tsent = 0;
            do {
                size_t chunk = MIN(len - tsent, PIPE_BUF);
                nsent = write(wrk.fd[1], ((char *) data) + tsent, chunk);
                if (nsent <= 0) {
                    int ws = 0;
                    if (errno == EAGAIN || errno == -EWOULDBLOCK) {
                        ws = waitWrite(wrk.fd[1]);
                        if (ws) {
                            lwarn(WLOG, "waiting to send message data to %hhu failed: %s", dst, errno_s);
                            return -1;
                        }
                        continue;
                    }

                    lwarn(WLOG, "sending message data to %hhu failed: %s", dst, errno_s);
                    return -1;
                }
                tsent += nsent;
            } while ((size_t) tsent < len);

            return tsent;
        }

        coroutine void asyncSend(Channel<int>& ch, uint8 dst, uint8 msg, const void *data, size_t len) {
            // send asynchronously
            ltrace(WLOG, "async_send ch %p, dst %hhu, msg 0x%hhX data %p, len %lu",
                   &ch, dst, msg, data, len);

            if (ipc::send(dst, msg, data, len) < 0) {
                lwarn(WLOG, "async send message 0x%hhX to %hhu failed", msg, dst);
            }

            if (ch) {
                ch << dst;
            }
        }

        coroutine void asyncBroadcast(uint8 msg, uint8 *data, size_t len)
        {
            // for each worker send the message
            int wait = 0;
            Channel<int> async(-1);

            // send asynchronously
            ltrace(WLOG, "async_broadcast msg %02X data %p, len %lu",
                            msg, data, len);

            for(uint8_t i = 1; i <= IPC->nworkers; i++) {
                if (i != spid && hasMessageHandler(i, msg)) {
                    // send to all other worker who are interested in the message
                    wait++;
                    go(asyncSend(async, i, msg, data, len));
                }
            }

            ltrace(WLOG, "waiting for %d messages to be sent, %lu",  wait, mnow());
            async(wait) | [&](bool, const int dst) {
                ltrace(WLOG, "message %02X sent to %hhu", msg, dst);
            };

            ltrace(WLOG, "messages sent, free data dup %p, %lu",  data, mnow());
            if (data)
                delete[] data;
        }

        void broadcast(uint8_t msg, const void *data, size_t len) {
            auto copy = Data(data, len, false).copy().release();
            ltrace(WLOG, "worker::broadcast dup %p msg %02X, data %p len %lu",
                   copy, msg, data, len);

            // start a go-routine to broadcast the message to all workers
            go(asyncBroadcast(msg, copy, len));
        }

        uint8 broadcast(Channel<int> &ch, uint8 msg, const void *data, size_t len) {
            // send message to all workers
            int wait = 0;

            ltrace(WLOG, "worker::broadcast ch %p, msg %02X, data %p len %lu",
                   &ch, msg, data, len);

            for(uint8 i = 1; i <= IPC->nworkers; i++) {
                if (i != spid && hasMessageHandler(i, msg)) {
                    // send to all other worker who are interested in the message
                    wait++;
                    go(asyncSend(ch, i, msg, data, len));
                }
            }

            strace("sent out %d %02X ipc broadcast messages", wait, msg);

            return (uint8_t) wait;
        }

        coroutine void asyncReceive(Worker& wrk) {
            // loop until the worker terminates
            int status = 0;

            ltrace(WLOG, "ipc receive loop starting");
            while (wrk.active) {
                status = receiveMessage(wrk, -1);
                if (status == EINVAL) {
                    lwarn(WLOG, "receiving message failed, aborting");
                    return;
                }
            }
            ltrace(WLOG, "ipc receive loop exiting %d, %d", status, wrk.active);
        }

        int receiveMessage(Worker& wrk, int64 dd) {
            if (!wrk.active) {
                lwarn(WLOG, "read on an inactive worker not supported");
                return ENOTSUP;
            }

            IPCMessageHeader hdr{0};
            ssize_t nread;
            int fd = wrk.fd[0];

            // read header
            while (true) {
                nread = ::read(fd, &hdr, sizeof(hdr));
                if (nread <= 0) {
                    int ws = errno;
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        ltrace(WLOG, "going to wait for ipc message hdr");
                        ws = waitRead(fd, dd);
                        if (ws == ETIMEDOUT) {
                            // it's ok to timeout
                            return ETIMEDOUT;
                        } else if (ws == 0) continue;
                    }

                    if (ws < 0 || errno == ECONNRESET || errno == EPIPE || nread == 0) {
                        ltrace(WLOG, "waiting for ipc header failed");
                        return EINVAL;
                    }

                    return errno;
                }
                break;
            }

            // header received, ensure that the message is supported
            if (!hasMessageHandler(wrk.id, hdr.id)) {
                ltrace(WLOG, "received unsupported ipc message %hhu", hdr.id);
                return 0;
            }

            ltrace(WLOG, "received header [%02X|%02X|%08X]", hdr.id, hdr.src, hdr.len);
            // receive message body
            uint8 *data{nullptr};
            bool allocd{false};
            size_t tread = 0;

            if (hdr.len) {
                if (hdr.len < 255) {
                    // no need to allocate memory for this
                    uint8 RX_BUF[256];
                    data = RX_BUF;
                }
                else {
                    data = new uint8[hdr.len];
                    allocd = true;
                }

                void *ptr = data;

                do {
                    size_t len = MIN(PIPE_BUF, (hdr.len - tread));
                    nread = read(fd, ((char *)ptr + tread), len);

                    if (nread <= 0) {
                        int ws = -errno;
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            ltrace(WLOG, "going to wait for ipc message hdr");
                            ws = waitRead(fd, dd);
                            if (ws == ETIMEDOUT) {
                                // it's ok to timeout
                                return ETIMEDOUT;
                            } else if (ws == 0) {
                                /* received read event */
                                continue;
                            }
                        }

                        if (ws < 0 || errno == ECONNRESET || errno == EPIPE || nread == 0) {
                            ltrace(WLOG, "waiting for ipc header failed: %s", errno_s);
                            return EINVAL;
                        }
                    }

                    tread += nread;
                } while (tread < hdr.len);
            }

            ltrace(WLOG, "received data [msg:%02X|src:%02X|len:%08X]", hdr.id, hdr.src, hdr.len);

            if (tread != hdr.len) {
                ldebug(WLOG, "reading message [msg:%02X|src:%02X|len:%08X] tread:%lu failed",
                       hdr.id, hdr.src, hdr.len, tread);
                return EINVAL;
            }

            // handle ipc message
            MessageHandler h = ipcHandlers[hdr.id];
            ltrace(WLOG, "invoking handler with [handler:%p|data:%p|len:%lu|allocd:%d",
                         h, data, tread, allocd);
            go(invokeHandler(h, hdr.src, data, tread, allocd));

            return 0;
        }

        int workerWait(bool quit) {
            int status;
            pid_t pid;

            if (quit)
                pid = waitpid(WAIT_ANY, &status, 0);
            else
                pid = waitpid(WAIT_ANY, &status, WNOHANG);

            if (pid == -1) {
                ltrace(WLOG, "waitpid() failed: %s", errno_s);
                return ECHILD;
            }

            if (pid == 0) return 0;

            for (uint8_t w = 1; w <= IPC->nworkers; w++) {
                Worker& wrk = IPC->workers[w];
                if (wrk.pid != pid) continue;

                ltrace(WLOG, "worker/%hhu exiting, status %d", wrk.id, status);
                wrk.active = false;
                break;
            }

            return 0;
        }

        void registerHandler(uint8 msg, MessageHandler handler) {

            if (msg <= IPC_MAX_NUMBER_OF_MESSAGES) {
                ipcHandlers[msg] = handler;
                if (spid == SPID_PARENT) {
                    for (uint8 w = 1; w <= IPC->nworkers; w++) {
                        setMessageHandler(w, msg, true);
                    }
                }
                else {
                    setMessageHandler(spid, msg, true);
                }
            }
            else
            lerror(WLOG, "registering a message handler for out of bound message");
        }

        void unRegisterHandler(uint8 msg) {
            if (msg <= IPC_MAX_NUMBER_OF_MESSAGES) {
                ipcHandlers[msg] = nullptr;
                if (spid == SPID_PARENT) {
                    for (uint8 w = 1; w <= IPC->nworkers; w++) {
                        setMessageHandler(w, msg, false);
                    }
                }
                else {
                    setMessageHandler(spid, msg, false);
                }
            }
            else
            lerror(WLOG, "un-registering a message handler for out of bound message");
        }

        bool spinLock(const uint8 idx, int64 tout) {
            assert(idx < WORKER_SHM_LOCKS);
            return Lock::spinLock(IPC->locks[idx], tout);
        }

        void spinUnlock(const uint8_t idx) {
            assert(idx < WORKER_SHM_LOCKS);
            return Lock::unlock(IPC->locks[idx]);
        }

        void registerCleaner(CleanUpHandler handler) {
            cleanupHandlers.insert(cleanupHandlers.begin(), std::move(handler));
        }

        Data get(uint8 msg, uint8 w, int64 tout) {
            if (w == SPID_PARENT || w == spid) {
                lerror(WLOG, "executing get to (%hhu) parent or current worker not supported",
                       w);
                return {};
            }

            IPCGetHandle async(0);
            IPCGetPayload payload{};
            
            auto& data = inflightGets.emplace(async.handle(), InflightGetData{}).first->second;
            data.reserve(1);

            auto handle = async.handle();
            defer({
                inflightGets.erase(handle);
            });

            payload.handle = async.handle();
            payload.deadline = tout <= 0? Deadline::Inf : mnow() + tout;
            payload.size = 0;
            ipc::send(w, msg, &payload, sizeof(payload));

            // receive the Response
            ltrace(WLOG, "sent get request waiting for 1 Response in %ld ms", tout);
            bool status = async[tout] >> w;

            if (status && !data.empty()) {
                /* successfully received data from worker */
                ltrace(WLOG, "got Response: data %p, size %lu", data.data(), data.size());
                return std::move(data[0]);
            } else {
                lerror(WLOG, "get from worker %hhu failed, msg %hhu", w, msg);
            }

            return {};
        }

        std::vector<Data> gather(uint8_t msg, int64_t tout) {
            if (IPC->nworkers == 0) {
                lwarn(WLOG, "executing get %hhu not supported when no workers", msg);
                return {};
            }

            if (ipc::spinLock(SHM_GATHER_LOCK, tout)) {
                /* buffered channel of up to 16 entries */
                IPCGetHandle async(0);
                IPCGetPayload payload{};

                auto& data = inflightGets.emplace(async.handle(), InflightGetData{}).first->second;
                data.reserve(IPC->nworkers-1);
                auto handle = async.handle();

                defer({ inflightGets.erase(handle); });

                payload.handle = async.handle();
                payload.deadline = tout <= 0? Deadline::Inf : mnow() + tout;
                payload.size = 0;

                ipc::broadcast(msg, &payload, sizeof(payload));

                ipc::spinUnlock(SHM_GATHER_LOCK);

                // wait for the response from all workers
                ltrace(WLOG, "sent gather request waiting for %hhu responses in %ld ms",
                       IPC->nactive-1, tout);

                bool status = async[tout](IPC->nactive-1) | Void;

                if (!status) {
                    lwarn(WLOG, "gather failed, msg %hhu", msg);
                }

                /* return gathered results */
                auto ret = std::move(data);
                for (auto& d: ret) {
                    auto s = hexstr(d.data(), d.size());
                    strace("data: " PRIs, _PRIs(s));
                }
                return std::move(ret);
            } else {
                lwarn(WLOG, "acquiring SHM_GET_LOCK timed out");
                return {};
            }

        }

        void sendGetResponse(const void *token, uint8 to, const void *data, size_t len) {
            ltrace(WLOG, "token %p, to %hhu, data %p, len %lu", token, to, data, len);
            if (token == nullptr) {
                lerror(WLOG, "invalid token %p", token);
                return;
            }

            void *resp;
            bool allocd{false};
            size_t tlen = len + sizeof(IPCGetPayload);
            if (len < 256) {
                static char SEND_BUF[256+(sizeof(IPCGetPayload))];
                resp = SEND_BUF;
            }
            else {
                allocd = true;
                resp = new uint8[tlen];
            }

            auto *payload = (IPCGetPayload *)resp;
            memcpy(resp, token, sizeof(IPCGetPayload));
            payload->size = len;
            memcpy(payload->data, data, len);
            ipc::send(to, GET_RESPONSE, resp, tlen);

            if (allocd) {
                /* free buffer if it was allocated */
                delete[] (uint8 *) resp;
            }
        }

        void registerGetHandler(uint8 msg, IPCGetHandler handler) {
            if (handler == nullptr) {
                lcritical(WLOG, "cannot register a null handler");
                return;
            }

            ipc::registerHandler(msg,
                           [handler](uint8_t src, uint8* data, size_t /*unused*/, bool /*unused*/) {
                               /* invoke the handler with the buffer as the token */
                               ltrace(WLOG, "invoking get handler [handler:%p|token:%p]", handler, data);
                               handler(data, src);
                               return false;
                           });
        }

        void registerGetResponse() {
            ipc::registerHandler(GET_RESPONSE,
                        [&](uint8_t src, uint8 *data, size_t len, bool allocd) {
                            auto *payload = (IPCGetPayload *) data;
                            /* go 50 ms ahead in time */
                            auto tmp = mnow() + 50;
                            if (payload->deadline == Deadline::Inf || payload->deadline > tmp) {
                                ltrace(WLOG, "got response in time deadline:%ld now:%ld",
                                        payload->deadline, tmp);
                                /*we haven't timed out waiting for Response */
                                chan handle = payload->handle;
                                auto it = inflightGets.find(handle);
                                if (it != inflightGets.end()) {
                                    auto s = hexstr(data, len);
                                    strace("data: " PRIs, _PRIs(s));
                                    auto s2 = hexstr(&data[sizeof(IPCGetPayload)-1], len-sizeof(IPCGetPayload));
                                    strace("data2: " PRIs, _PRIs(s2));
                                    it->second.push_back(
                                            Data{data, len-sizeof(IPCGetPayload), allocd}
                                            .own(sizeof(IPCGetPayload)-1));
                                    chs(handle, uint8, src);
                                    return true;
                                }
                                else {
                                    ltrace(WLOG,
                                        "got response from %hhu without worker", src);
                                }
                            }
                            else {
                                lwarn(WLOG, "got response from %hhu after timeout dd %ld now %ld",
                                        src, payload->deadline, tmp);
                            }
                            return false;
                        });
        }

        coroutine void invokeHandler(MessageHandler handler, uint8 src, uint8 *data, size_t len, bool own)
        {
            try {
                if (!handler(src, data, len, own) && own)
                    delete[] data;
            } catch(std::exception& ex) {
                lwarn(WLOG, "un-handled exception in msg handler: %s", ex.what());
            }
        }
    }
}