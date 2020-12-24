//
// Created by Mpho Mbotho on 2020-11-29.
//

#ifndef SUILRPC_SIGNAL_HPP
#define SUILRPC_SIGNAL_HPP

#include <suil/base/utils.hpp>

#include <memory>
#include <list>

namespace suil {

    template <typename S>
    class Connection;

    template <typename S>
    class Signal;

    /**
     * An observer is object used to observe an observable
     *
     * @tparam Func the function that will be invoked when data is
     * published on an observable
     */
    template <typename R, typename... Args>
        requires std::is_void_v<R>
    class Connection<R(Args...)> {
    public:
        using Func = std::function<R(Args...)>;

        void disconnect() {
            if (func != nullptr) {
                func = nullptr;
            }
        }

        operator bool() const { return  func != nullptr; }

    private:
        template <typename T>
        friend class Signal;

        explicit Connection(Func&& func)
            : func{std::make_shared<Function>(std::move(func))}
        {}

        struct Function {

            DISABLE_MOVE(Function);
            DISABLE_COPY(Function);

            explicit Function(Func&& func)
                : fn{std::move(func)}
            {}

            void operator()(Args... args)
            {
                if (fn) {
                    std::invoke(fn, args...);
                }
            }

            Func fn{};
        };

        std::shared_ptr<Function> func{nullptr};
    };

    /**
     * A signal is an object which can be observed by different functions
     * @tparam Func The type of function that is allowed to observe observables
     * of this type
     */
    template <typename R, typename... Args>
        requires std::is_void_v<R>
    class Signal<R(Args...)> {
        using Callback = std::weak_ptr<typename Connection<R(Args...)>::Function>;
        using LifeTimeObserver = std::shared_ptr<Connection<R(Args...)>>;
    public:
        using Func = std::function<R(Args...)>;
        /**
         * This wraps around a connection object that is registered as a lifetime
         * observer. Lifetime connections will live as long as the signal
         * is alive or are explicitly canceled. This id can be used to cancel
         * a lifetime connection
         */
        class ConnectionId {
        public:
            operator bool () const {
                return !mConn.expired();
            }

        private:
            template <typename T>
            friend class Signal;
            explicit ConnectionId(std::weak_ptr<Connection<R(Args...)>> conn)
                : mConn{std::move(conn)}
            {}
            std::weak_ptr<Connection<R(Args...)>> mConn;
        };

        /**
         * Register a function with the signal
         * @param func The function that will be invoked when data is published
         * into the observable
         * @return On observer object
         */
        Connection<R(Args...)> connect(Func func)
        {
            Connection<R(Args...)> conn(std::move(func));
            callbacks.emplace_back(conn.func);
            return conn;
        }

        /**
         * Register a function with the signal. This function will remain registered
         * until the signal is destroyed or explicitly canceled
         * @param func the function to register with the signal
         * @return  An ID which can later be used to cancel the lifetime connection
         */
        ConnectionId operator+=(Func func) {
            auto conn = LifeTimeObserver{new Connection<R(Args...)>(std::move(func))};
            callbacks.emplace_back(conn->func);
            lifeTimeObservers.push_back(conn);
            return ConnectionId{conn};
        }

        /**
         * Cancel an observer using a previously returned ID
         * @param id  the ID of the observer
         */
        void disconnect(ConnectionId& id)
        {
            if (auto connId = id.mConn.lock()) {
                auto it = lifeTimeObservers.begin();
                while (it != lifeTimeObservers.end()) {
                    if ((*it).get() ==  connId.get()) {
                        it = lifeTimeObservers.erase(it);
                    }
                    else {
                        it = std::next(it, 1);
                    }
                }
            }
        }

        void operator()(Args... args)
        {
            auto it = callbacks.begin();
            while (it != callbacks.end()) {
                // invoke all callbacks
                auto& cb = *it;
                if (auto callback = cb.lock()) {
                    (*callback)(args...);
                    it = std::next(it, 1);
                    continue;
                }

                // erase obsolete callbacks
                it = callbacks.erase(it);
            }
        }

        inline operator bool() const {
            return !Ego.callbacks.empty();
        }

    private suil_ut:
        std::list<Callback> callbacks;
        // Life observers
        std::vector<LifeTimeObserver> lifeTimeObservers;
    };
}
#endif //SUILRPC_OBSERVABLE_HPP
