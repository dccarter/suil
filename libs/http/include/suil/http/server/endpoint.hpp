//
// Created by Mpho Mbotho on 2020-12-18.
//

#ifndef SUIL_HTTP_SERVER_ENDPOINT_HPP
#define SUIL_HTTP_SERVER_ENDPOINT_HPP

#include <suil/http/server/connection.hpp>

#include <suil/net/server.hpp>
#include <suil/base/thread.hpp>

namespace suil::http::server {

    define_log_tag(HTTP_SERVER);

    template <typename T>
    concept HasDependencies = requires(T t) {
        typename T::Depends;
    };

    template <typename ...Mws>
    class EndpointContext;

    template <typename... Mws>
    class ConnectionWorker : public ThreadExecutor<net::Socket> {
    public:
        ConnectionWorker(ThreadPool<net::Socket>& pool, int index, EndpointContext<Mws...>& ctx)
            : ThreadExecutor<net::Socket>(pool, index),
              _ctx{ctx}
        {}

        void executeWork(Work &work) override
        {
            Connection<Mws...> conn{
                            work,
                            _ctx.getConfig(),
                            _ctx.getRouter(),
                            &_ctx.middlewares(),
                            _ctx.getStats()};
            conn.start();
        }

    private:
        EndpointContext<Mws...>& _ctx;
    };

    template <typename ...Mws>
    class EndpointContext {
        template <typename ...M>
        friend class Endpoint;
        template <typename... M>
        friend class ConnectionWorker;

        template <typename ...Opts>
        EndpointContext(String api, Opts&&... opts)
            : _router{std::move(api)},
              _pool{"HttpConnection"}
        {
            suil::applyConfig(_config, std::forward<Opts>(opts)...);
            if (_config.keepAliveTime > 0) {
                // change connection timeout match the keep alive time
                _config.connectionTimeout = _config.keepAliveTime;
            }
            _stats = HttpServerStats{};
            _pool.setNumberOfWorkers(_config.numberOfWorkers)
                 .setWorkerBackoffWaterMarks(
                         _config.threadPoolBackoffHigh,
                         _config.threadPoolBackoffLow);
        }

        inline const HttpServerConfig& getConfig() const {
            return _config;
        }

        inline HttpServerConfig& getConfig() {
            return _config;
        }

        inline const HttpServerStats& getStats() const {
            return _stats;
        }

        inline HttpServerStats& getStats() {
            return _stats;
        }

        inline const Router& getRouter() const {
            return _router;
        }

        inline Router& getRouter() {
            return _router;
        }

        inline const std::tuple<Mws...>& middlewares() const {
            return _mws;
        }

        inline std::tuple<Mws...>& middlewares() {
            return _mws;
        }

    private:
        HttpServerConfig _config{};
        HttpServerStats  _stats{};
        Router           _router;
        std::tuple<Mws...> _mws{};
        ThreadPool<net::Socket> _pool;

        inline ThreadPool<net::Socket>& getPool() {
            return _pool;
        }
    };

    template <typename ...Mws>
    class Endpoint : public LOGGER(HTTP_SERVER) {
    public:
        using Context = EndpointContext<Mws...>;
        using Self = Endpoint<Mws...>;
        using Middlewares = std::tuple<Mws...>;

        struct ConnectionHandler : public net::CustomSchedulingHandler {
            void operator()(net::Socket::UPtr&& sock, Context& ctx)
            {
                ctx.getPool().schedule(std::move(sock));
            }
        };

        using Backend = net::Server<ConnectionHandler, Context>;

        template <typename ...Opts>
        Endpoint(const String& api, Opts&&... opts)
            : _ctx{new Context(api.dup(), std::forward<Opts>(opts)...)}
        {
            _backend = std::make_unique<Backend>(
                                _ctx->getConfig().serverConfig,
                                _ctx);
        }

        inline int listen() {
            return backend().listen();
        }

        int start() {
            if (_backend and backend().isRunning()) {
                iwarn("Server backend already running, will not be restarted");
                return EINPROGRESS;
            }
            using Executor = ConnectionWorker<Mws...>;
            _ctx->getPool().template start<Executor>(*_ctx);
            router().validate();
            return backend().start();
        }

        void stop() {
            backend().stop();
            // stop the thread pool
            _ctx->getPool().stop();
        }

        template <typename... Opts>
        inline void backendConfigure(Opts... opts) {
            suil::applyConfig(config().serverConfig, std::forward<Opts>(opts)...);
        }

        template <typename... Opts>
        inline void configure(Opts... opts) {
            suil::applyConfig(config().serverConfig, std::forward<Opts>(opts)...);
        }

        inline DynamicRule& dynamic(const String& rule) {
            return router().createDynamicRule(rule);
        }

        inline DynamicRule& operator()(const String& rule) {
            return dynamic(rule);
        }

        template <uint64 Tag>
        typename std::result_of<decltype(&Router::createTaggedRule<Tag>)(Router, const String&)>::type
        route(const String& rule)
        {
            return router().template createTaggedRule<Tag>(rule);
        }

        using MwContext = crow::Context<Mws...>;

        template <typename T>
            requires crow::magic::contains<T, Mws...>::value
        typename T::Context& context(const Request& req)
        {
            auto& ctx = *reinterpret_cast<MwContext*>(req.middlewareContent);
            return ctx.template get<T>();
        }

        template <typename T>
            requires crow::magic::contains<T, Mws...>::value
        T& middleware() {
            return crow::get_element_by_type<T, Mws...>(_ctx->middlewares());
        }

        inline const HttpServerConfig& config() const {
            return _ctx->getConfig();
        }

        inline HttpServerStats& stats() {
            return _ctx->getStats();
        }

        inline const String& apiBaseRoute() const {
            return router().apiBase();
        }

        inline Router& router() {
            return _ctx->getRouter();
        }

        inline const Router& router() const {
            return _ctx->getRouter();
        }

    private:
        Backend& backend() {
            if (!_backend) {
                throw UnsupportedOperation("Server backend is null");
            }

            return *_backend;
        }

        inline HttpServerConfig& config() {
            return _ctx->getConfig();
        }

        std::shared_ptr<Context> _ctx{nullptr};
        std::unique_ptr<Backend> _backend{nullptr};
    };

#define Route(app, url) \
            app.route<suil::http::crow::magic::get_parameter_tag(url)>(url)

}

constexpr suil::http::Method operator "" _method(const char* str, size_t /*len*/) {
    return
            suil::http::crow::magic::is_equ_p(str, "GET", 3) ?     suil::http::Method::Get :
            suil::http::crow::magic::is_equ_p(str, "DELETE", 6) ?  suil::http::Method::Delete :
            suil::http::crow::magic::is_equ_p(str, "HEAD", 4) ?    suil::http::Method::Head :
            suil::http::crow::magic::is_equ_p(str, "POST", 4) ?    suil::http::Method::Post :
            suil::http::crow::magic::is_equ_p(str, "PUT", 3) ?     suil::http::Method::Put :
            suil::http::crow::magic::is_equ_p(str, "OPTIONS", 7) ? suil::http::Method::Options :
            suil::http::crow::magic::is_equ_p(str, "CONNECT", 7) ? suil::http::Method::Connect :
            suil::http::crow::magic::is_equ_p(str, "TRACE", 5) ?   suil::http::Method::Trace :
            throw suil::InvalidArguments("invalid http method");
}

#endif //SUIL_HTTP_SERVER_ENDPOINT_HPP
