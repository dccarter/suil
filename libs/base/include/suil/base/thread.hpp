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

    template <typename Executor>
    class ThreadPool;

    define_log_tag(THRD_EXECUTOR);

    template <typename W>
    class ThreadExecutor : public LOGGER(THRD_EXECUTOR) {
    public:
        using Work = W;
        using Ptr  = std::unique_ptr<ThreadExecutor<Work>>;
        ThreadExecutor(ThreadPool<W>& pool, int index)
            : _pool(pool),
              _index{index}
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
                _workInProgress++;
                if (_pool.isBackoffEnabled() and (_workInProgress >= _pool.backoffHigh())) {
                    _backoff = true;
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
                idebug("Starting thread pool executor " PRIs "/%d",
                           _PRIs(_pool.name()), _index);
                executeLoop();
                idebug(PRIs "/%d thread pool executor done",
                                _PRIs(_pool.name()), _index);
            });
        }

        virtual void executeLoop()
        {

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
                    work = std::move(_workQueue.front());
                    _workQueue.pop_front();
                }
                go(runWork(Ego, std::move(work)));
            }
        }

        void stop()
        {
            if (!_stopped) {
                _stopped = true;
                // tell dispatcher to stop if it is working
                _waitingEvt.notify();
            }

            if (_th.joinable()) {
                _th.join();
            }
        }


        inline virtual ~ThreadExecutor() {
            Ego.stop();
        }

        inline int index() const {
            return _index;
        }

    protected:
        virtual void executeWork(Work& work) = 0;

        std::deque<std::unique_ptr<Work>> _workQueue{};
        mill::Mutex  _wqMutex{};
        mill::Event  _waitingEvt{};
        std::thread _th;
        std::atomic_bool _stopped{false};
        std::atomic_bool _backoff{false};
        std::atomic_uint32_t _workInProgress{0};
        ThreadPool<W>& _pool;
        int _index{0};

    private:
        void onWorkDone()
        {
            _workInProgress--;
            if (_backoff and (_workInProgress <= _pool.backoffLow())) {
                // pressure released, notify to pool we are ready for work
                _pool.notifyExecutorReady(Ego);
            }
        }

        static coroutine void runWork(ThreadExecutor& S, std::unique_ptr<Work>&& work)
        {
            auto wrk = std::move(work);
            S.executeWork(*wrk);
            S.onWorkDone();
        }
    };

    define_log_tag(THRD_POOL);
    template <typename Work>
    class ThreadPool : LOGGER(THRD_POOL) {
    public:
        using Executor = ThreadExecutor<Work>;

        explicit ThreadPool(String name)
            : _name{std::move(name)}
        {}

        ThreadPool(ThreadPool&& o) noexcept
            : _executors{std::move(o._executors)},
              _nExecutors{std::exchange(o._nExecutors, 0)},
              _backoffLow{std::exchange(o._backoffLow, 0)},
              _backoffHigh{std::exchange(o._backoffHigh, 0)},
              _nextExecutorIndex{std::exchange(o._nextExecutorIndex, 0)},
              _backoffEvent{std::move(o._backoffEvent)},
              _sync{std::move(o._sync)},
              _name{std::move(o._name)}
        {}

        ThreadPool& operator=(ThreadPool&& o)
        {
            if (&o == this) {
                return Ego;
            }

            _executors = std::move(o._executors);
            _nExecutors = std::exchange(o._nExecutors, 0);
            _backoffLow = std::exchange(o._backoffLow, 0);
            _backoffHigh = std::exchange(o._backoffHigh, 0);
            _nextExecutorIndex = std::exchange(o._nextExecutorIndex, 0);
            _backoffEvent = std::move(o._backoffEvent);
            _sync = std::move(o._sync);
            _name = std::move(o._name);
            return Ego;
        }

        template <typename Exec, typename... Args>
        void start(Args&&... args)
        {
            if (_executors.empty()) {
                iinfo("Thread pool " PRIs " starting %u executors", _PRIs(_name), _nExecutors);
                // thread pool not initialized
                initializeWorkers<Exec>(std::forward<Args>(args)...);
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
                int i = _nextExecutorIndex;
                do {
                    if (i == _executors.size())
                        i = 0;
                    auto& worker = _executors[i];
                    if (worker->queueWork(work)) {
                        _nextExecutorIndex = (i+1) % _executors.size();
                        return;
                    }
                } while(++i != _nextExecutorIndex);

                // Wait for any worker to be ready to receive work
                idebug("All workers in backoff globalState, waiting for a worker to be available");
                _backoffEvent.wait(lk);
            }
        }

        inline ThreadPool<Work>& setNumberOfWorkers(uint16 workers) {
            _nExecutors = workers;
            return Ego;
        }

        inline ThreadPool<Work>& setWorkerBackoffWaterMarks(uint32 high, uint32 low) {
            _backoffLow = low;
            _backoffHigh = high;
            return Ego;
        }

        inline uint16 backoffLow() const {
            return _backoffLow;
        }

        inline uint16 backoffHigh() const {
            return _backoffHigh;
        }

        inline const String& name() const {
            return _name;
        }

        inline bool isBackoffEnabled() const {
            return _backoffLow != _backoffHigh;
        }

        inline void notifyExecutorReady(Executor& w) {
            if (w.index() >= _executors.size()) {
                iwarn(PRIs " notifying worker (%d) is not registered in pool",
                                _PRIs(_name), w.index());
            }
            _nextExecutorIndex = w.index();
            _backoffEvent.notify();
        }

    private:
        DISABLE_COPY(ThreadPool);
        template <typename Exec, typename... Args>
        void initializeWorkers(Args&&... args)
        {
            static const auto nCores = std::thread::hardware_concurrency();
            if (_nExecutors == 0) {
                // default  to number of CPU cores
                idebug("pool '" PRIs "' - defaulting number of executors to number of CPU cores {%u}",
                            _PRIs(_name), nCores);
                _nExecutors = nCores - 1;
            }

            if (_nExecutors > nCores) {
                // We really should limit our number of threads to number of CPU cores
                iwarn("pool '" PRIs "' - number of requested threads {%u} exceeds number CPU cores{%u}",
                      _PRIs(_name), _nExecutors, nCores);
            }
            if (_backoffLow > _backoffHigh) {
                iwarn("pool '" PRIs "' - backoffLow(=%hu) > backoffHigh(=%hu) resetting to 200/256",
                      _PRIs(_name), _backoffLow, _backoffHigh);
                setWorkerBackoffWaterMarks(256, 200);
            }

            _executors.reserve(_nExecutors);
            for (int i = 0; i < _nExecutors; i++) {
                _executors.emplace_back(
                        std::make_unique<Exec>(
                                Ego, i,
                                std::forward<Args>(args)...));

                _executors.back()->init();
                _executors.back()->run();
            }
        }

        std::vector<typename Executor::Ptr> _executors;
        uint16       _nExecutors{1};
        uint32       _backoffLow{1000};
        uint32       _backoffHigh{1000};
        uint32       _nextExecutorIndex{0};
        mill::Event  _backoffEvent{};
        mill::Mutex  _sync{};
        String       _name{};
    };
}
#endif //SUIL_BASE_THREAD_HPP
