//
// Created by Mpho Mbotho on 2021-01-10.
//

#ifndef SUIL_BASE_THREAD_HPP
#define SUIL_BASE_THREAD_HPP

#include <suil/base/utils.hpp>
#include <suil/base/logging.hpp>
#include <suil/base/string.hpp>

#include <libmill/libmill.hpp>
#include <queue>

namespace suil {

    template <typename W>
    class WorkerThread {
    public:
        using Work = W;
        using Ptr  = std::unique_ptr<WorkerThread<Work>>;
        WorkerThread(mill::Event& poolSync, uint32 backoffLow, uint32 backoffHigh)
            : _poolEvt{poolSync},
              _backoffLow{backoffLow},
              _backoffHigh{backoffHigh}
        {}

        bool queueWork(std::unique_ptr<Work>& work)
        {
            if (_stopped) {
                return false;
            }
            if (_backoff) {
                return false;
            }

            {
                mill::Lock lk{_wqMutex};
                _workQueue.push_back(std::move(work));
                // sdebug("%u -> %u/%zu", mtid(), _workInProgress.load(), _workQueue.size());
                if (++_workInProgress >= _backoffHigh) {
                    _backoff = _backoffLow != _backoffHigh;
                }
            }
            _waitingEvt.notify();
            return true;
        }

        virtual void init() {}

        void run()
        {
            // run work queue until explicitly stopped
            _stopped = false;
            _th = masync([this] {
                while (!_stopped) {
                    std::unique_ptr<Work> work{};
                    // wait for events and dispatch
                    {
                        mill::Lock lk{_wqMutex};
                        if (_workQueue.empty()) {
                            // wait until there is an event or the dispatcher is aborted
                            _waitingEvt.wait(lk);
                            continue;
                        }
                        // sdebug("%u -> %u/%zu", mtid(), _workInProgress.load(), _workQueue.size());
                        work = std::move(_workQueue.front());
                        _workQueue.pop_front();
                    }
                    go(runWork(Ego, std::move(work)));
                }
            });
        }

        void stop()
        {
            if (!_stopped) {
                _stopped = true;
                // tell dispatcher to stop if it is working
                _waitingEvt.notify();
            }
        }


        inline virtual ~WorkerThread() {
            Ego.stop();
            if (_th.joinable()) {
                _th.join();
            }
        }

    protected:
        virtual void executeWork(Work& work) = 0;

    private:
        static coroutine void runWork(WorkerThread& S, std::unique_ptr<Work>&& work)
        {
            auto wrk = std::move(work);
            S.executeWork(*wrk);
            S._workInProgress--;
            if (S._backoff and S._workInProgress <= S._backoffLow) {
                // turn off backoff and notify producer that we are ready for work
                S._backoff = false;
                S._poolEvt.notifyOne();
            }
        }

        std::deque<std::unique_ptr<Work>> _workQueue{};
        mill::Mutex  _wqMutex{};
        mill::Event  _waitingEvt{};
        mill::Event& _poolEvt;
        uint32 _backoffLow{0};
        uint32 _backoffHigh{0};
        std::thread _th;
        std::atomic_bool _stopped{false};
        std::atomic_bool _backoff{false};
        std::atomic_uint32_t _workInProgress{0};
    };

    define_log_tag(THRD_POOL);
    template <typename Worker>
    class ThreadPool : LOGGER(THRD_POOL) {
    public:
        using Work = typename Worker::Work;

        explicit ThreadPool(String name)
            : _name{std::move(name)}
        {}

        ThreadPool(ThreadPool&& o) noexcept
            : _workers{std::move(o._workers)},
              _nWorkers{std::exchange(o._nWorkers, 0)},
              _workBackoffLow{std::exchange(o._workBackoffLow, 0)},
              _workBackoffHigh{std::exchange(o._workBackoffHigh, 0)},
              _lastWorkerIndex{std::exchange(o._lastWorkerIndex, 0)},
              _waitWorker{std::move(o._waitWorker)},
              _sync{std::move(o._sync)},
              _name{std::move(o._name)}
        {}

        ThreadPool& operator=(ThreadPool&& o)
        {
            if (&o == this) {
                return Ego;
            }

            _workers = std::move(o._workers);
            _nWorkers = std::exchange(o._nWorkers, 0);
            _workBackoffLow = std::exchange(o._workBackoffLow, 0);
            _workBackoffHigh = std::exchange(o._workBackoffHigh, 0);
            _lastWorkerIndex = std::exchange(o._lastWorkerIndex, 0);
            _waitWorker = std::move(o._waitWorker);
            _sync = std::move(o._sync);
            _name = std::move(o._name);
            return Ego;
        }

        template <typename... Args>
        void start(Args&&... args)
        {
            if (_workers.empty()) {
                // thread pool not initialized
                initializeWorkers(std::forward<Args>(args)...);
            }
        }

        template <typename ...Args>
        inline void emplace(Args&&... args)
        {
            schedule(std::make_unique<Work>(std::forward<Args>(args)...));

        }

        void schedule(std::unique_ptr<Work>&& work)
        {
            mill::Lock lk{_sync};
            while (true) {
                // find any worker that is not in backoff state
                int i = _lastWorkerIndex+1;
                int end = (_lastWorkerIndex+1)%_workers.size();
                do {
                    if (i == _workers.size())
                        i = 0;
                    auto& worker = _workers[i];
                    if (worker->queueWork(work)) {
                        _lastWorkerIndex = i;
                        return;
                    }
                } while(++i != end);

                // Wait for any worker to be ready to receive work
                idebug("All workers in backoff state, waiting for a worker to be available");
                _waitWorker.wait(lk);
            }
        }

        inline ThreadPool<Worker>& setNumberOfWorkers(uint16 workers) {
            _nWorkers = workers;
            return Ego;
        }

        inline ThreadPool<Worker>& setWorkerBackoffWaterMarks(uint32 high, uint32 low) {
            _workBackoffLow = low;
            _workBackoffHigh = high;
            return Ego;
        }

    private:
        DISABLE_COPY(ThreadPool);
        template <typename... Args>
        void initializeWorkers(Args&&... args)
        {
            static const auto nCores = std::thread::hardware_concurrency();
            if (_nWorkers > nCores) {
                // We really should limit our number of threads to number of CPU cores
                iwarn("pool '" PRIs "' - number of requested threads {%u} exceeds number CPU cores{%u}",
                      _PRIs(_name), _nWorkers, nCores);
            }
            _workers.reserve(_nWorkers);
            for (int i = 0; i < _nWorkers; i++) {
                _workers.emplace_back(
                        std::make_unique<Worker>(
                                _waitWorker,
                                _workBackoffLow,
                                _workBackoffHigh,
                                std::forward<Args>(args)...));
                _workers.back()->init();
                _workers.back()->run();
            }
        }

        std::vector<typename Worker::Ptr> _workers;
        uint16       _nWorkers{1};
        uint32       _workBackoffLow{1000};
        uint32       _workBackoffHigh{1000};
        uint32       _lastWorkerIndex{0};
        mill::Event  _waitWorker{};
        mill::Mutex  _sync{};
        String      _name{};
    };
}
#endif //SUIL_BASE_THREAD_HPP
