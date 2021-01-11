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
    class ConnectionWorker : public WorkerThread<net::Socket>, LOGGER(HTTP_SERVER) {
    public:
        using WorkerThread<net::Socket>::WorkerThread;
        ConnectionWorker(
                mill::Event& poolSync,
                uint32 backoffLow,
                uint32 backoffHigh,
                std::shared_ptr<EndpointContext<Mws...>> ctx)
            : WorkerThread<net::Socket>(poolSync, backoffLow, backoffHigh),
              ctx{ctx}
        {}

        void executeWork(Work &work) override
        {
            Connection<Mws...> conn{work, ctx->_config, ctx->_router, &ctx->_mws, ctx->_stats};
            conn.start();
        }

    private:
        std::shared_ptr<EndpointContext<Mws...>> ctx;
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
            suil::applyConfig(Ego._config, std::forward<Opts>(opts)...);
            if (Ego._config.keepAliveTime > 0) {
                // change connection timeout match the keep alive time
                Ego._config.connectionTimeout = Ego._config.keepAliveTime;
            }
            Ego._stats = HttpServerStats{};
            _pool.setNumberOfWorkers(_config.numberOfWorkers)
                 .setWorkerBackoffWaterMarks(
                         _config.threadPoolBackoffHigh,
                         _config.threadPoolBackoffLow);
        }

        HttpServerConfig _config{};
        HttpServerStats  _stats{};
        Router           _router;
        std::tuple<Mws...> _mws{};
        ThreadPool<ConnectionWorker<Mws...>> _pool;

        inline ThreadPool<ConnectionWorker<Mws...>>& getPool() {
            return _pool;
        }
    };

    template <typename ...Mws>
    class Endpoint : public LOGGER(HTTP_SERVER) {
    public:
        using Context = EndpointContext<Mws...>;
        using Self = Endpoint<Mws...>;
        using Middlewares = std::tuple<Mws...>;

        struct ConnectionHandler : public net::BlockingHandler {
            void operator()(net::Socket::UPtr&& sock, std::shared_ptr<Context> ctx)
            {
                ctx->getPool().schedule(std::move(sock));
            }
        };

        using Backend = net::Server<ConnectionHandler, Context>;

        template <typename ...Opts>
        Endpoint(const String& api, Opts&&... opts)
            : _ctx{new Context(api.dup(), std::forward<Opts>(opts)...)}
        {
            Ego._backend = std::make_unique<Backend>(
                                Ego._ctx->_config.serverConfig,
                                Ego._ctx);
        }

        inline int listen() {
            return Ego.backend().listen();
        }

        int start() {
            _ctx->getPool().start(_ctx);
            Ego.router().validate();
            itrace("starting server....");
            return Ego.backend().start();
        }

        inline void stop() {
            return Ego.backend().stop();
        }

        template <typename... Opts>
        inline void backendConfigure(Opts... opts) {
            suil::applyConfig(Ego.config().serverConfig, std::forward<Opts>(opts)...);
        }

        template <typename... Opts>
        inline void configure(Opts... opts) {
            suil::applyConfig(Ego.config().serverConfig, std::forward<Opts>(opts)...);
        }

        inline DynamicRule& dynamic(const String& rule) {
            return Ego.router().createDynamicRule(rule);
        }

        inline DynamicRule& operator()(const String& rule) {
            return Ego.dynamic(rule);
        }

        template <uint64 Tag>
        typename std::result_of<decltype(&Router::createTaggedRule<Tag>)(Router, const String&)>::type
        route(const String& rule)
        {
            return Ego.router().template createTaggedRule<Tag>(rule);
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
            return crow::get_element_by_type<T, Mws...>(Ego._ctx->_mws);
        }

        inline const HttpServerConfig& config() const {
            return Ego._ctx->_config;
        }

        inline HttpServerStats& stats() {
            return Ego._ctx->_stats;
        }

        inline const String& apiBaseRoute() const {
            return Ego.router().apiBase();
        }

        inline Router& router() {
            return Ego._ctx->_router;
        }

        inline const Router& router() const {
            return Ego._ctx->_router;
        }

    private:
        Backend& backend() {
            if (!Ego._backend) {
                throw UnsupportedOperation("Server backend is null");
            }

            return *Ego._backend;
        }

        inline HttpServerConfig& config() {
            return Ego._ctx->_config;
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
