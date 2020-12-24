//
// Created by Mpho Mbotho on 2020-12-18.
//

#ifndef SUIL_HTTP_SERVER_CONNECTION_HPP
#define SUIL_HTTP_SERVER_CONNECTION_HPP

#include <suil/http/server/router.hpp>

namespace suil::http {
    namespace crow {

        template <typename ... Middlewares>
        struct PartialContext
                : public magic::pop_back<Middlewares...>::template rebind<PartialContext>,
                  public magic::last_element_type<Middlewares...>::type::Context
        {
            using ParentContext = typename magic::pop_back<Middlewares...>::template rebind<crow::PartialContext>;
            template <int N>
            using Partial = typename std::conditional<N == sizeof...(Middlewares)-1, PartialContext, typename ParentContext::template Partial<N>>::type;

            template <typename T>
            typename T::Context& get()
            {
                return static_cast<typename T::context&>(*this);
            }
        };

        template <>
        struct PartialContext<>
        {
            template <int>
            using Partial = PartialContext;
        };

        template <int N, typename Context, typename Container, typename CurrentMW, typename ... Middlewares>
        bool middlewareCallHelper(Container& middlewares, server::Request& req, server::Response& res, Context& ctx);

        template <typename ... Middlewares>
        struct Context : private PartialContext<Middlewares...>
            //struct context : private Middlewares::context... // simple but less type-safe
        {
            template <int N, typename Context, typename Container>
            friend typename std::enable_if<(N==0)>::type afterHandlersCallHelper(Container& middlewares, Context& ctx, server::Request& req, server::Response& res);
            template <int N, typename Context, typename Container>
            friend typename std::enable_if<(N>0)>::type afterHandlersCallHelper(Container& middlewares, Context& ctx, server::Request& req, server::Response& res);

            template <int N, typename Context, typename Container, typename CurrentMW, typename ... Middlewares2>
            friend bool middlewareCallHelper(Container& middlewares, server::Request& req, server::Response& res, Context& ctx);

            template <typename T>
            typename T::Context& get()
            {
                return static_cast<typename T::Context&>(*this);
            }

            template <int N>
            using Partial = typename PartialContext<Middlewares...>::template Partial<N>;
        };

        template<typename MW>
        struct check_before_handle_arity_3_const {
            template<typename T,
                    void (T::*)(server::Request &, server::Response &, typename MW::Context &) const = &T::before
            >
            struct get {
            };
        };

        template<typename MW>
        struct check_before_handle_arity_3 {
            template<typename T,
                    void (T::*)(server::Request &, server::Response &, typename MW::Context &) = &T::before
            >
            struct get {
            };
        };

        template<typename MW>
        struct check_after_handle_arity_3_const {
            template<typename T,
                    void (T::*)(server::Request &, server::Response &, typename MW::Context &) const = &T::after
            >
            struct get {
            };
        };

        template<typename MW>
        struct check_after_handle_arity_3 {
            template<typename T,
                    void (T::*)(server::Request &, server::Response &, typename MW::Context &) = &T::after
            >
            struct get {
            };
        };

        template<typename T>
        struct is_before_handle_arity_3_impl {
            template<typename C>
            static std::true_type f(typename check_before_handle_arity_3_const<T>::template get<C> *);

            template<typename C>
            static std::true_type f(typename check_before_handle_arity_3<T>::template get<C> *);

            template<typename C>
            static std::false_type f(...);

        public:
            static const bool value = decltype(f<T>(nullptr))::value;
        };

        template<typename T>
        struct is_after_handle_arity_3_impl {
            template<typename C>
            static std::true_type f(typename check_after_handle_arity_3_const<T>::template get<C> *);

            template<typename C>
            static std::true_type f(typename check_after_handle_arity_3<T>::template get<C> *);

            template<typename C>
            static std::false_type f(...);

        public:
            static const bool value = decltype(f<T>(nullptr))
            ::value;
        };

        template<typename MW, typename Context, typename ParentContext>
        typename std::enable_if<!is_before_handle_arity_3_impl<MW>::value>::type
        before_handler_call(MW &mw, server::Request &req, server::Response &res, Context &ctx, ParentContext & /*parent_ctx*/) {
            mw.before(req, res, ctx.template get<MW>(), ctx);
        }

        template<typename MW, typename Context, typename ParentContext>
        typename std::enable_if<is_before_handle_arity_3_impl<MW>::value>::type
        before_handler_call(MW &mw, server::Request &req, server::Response &res, Context &ctx, ParentContext & /*parent_ctx*/) {
            mw.before(req, res, ctx.template get<MW>());
        }

        template<typename MW, typename Context, typename ParentContext>
        typename std::enable_if<!is_after_handle_arity_3_impl<MW>::value>::type
        after_handler_call(MW &mw, server::Request &req, server::Response &res, Context &ctx, ParentContext & /*parent_ctx*/) {
            mw.after(req, res, ctx.template get<MW>(), ctx);
        }

        template<typename MW, typename Context, typename ParentContext>
        typename std::enable_if<is_after_handle_arity_3_impl<MW>::value>::type
        after_handler_call(MW &mw, server::Request &req, server::Response &res, Context &ctx, ParentContext & /*parent_ctx*/) {
            mw.after(req, res, ctx.template get<MW>());
        }

        template<int N, typename Context, typename Container, typename CurrentMW, typename ... Middlewares>
        bool middlewareCallHelper(Container &middlewares, server::Request &req, server::Response &res, Context &ctx) {
            using ParentContext_t = typename Context::template Partial<N - 1>;
            before_handler_call<CurrentMW, Context, ParentContext_t>(std::get<N>(middlewares), req, res, ctx,
                                                                      static_cast<ParentContext_t &>(ctx));

            if (res.isCompleted()) {
                after_handler_call<CurrentMW, Context, ParentContext_t>(std::get<N>(middlewares), req, res,
                                                                         ctx,
                                                                         static_cast<ParentContext_t &>(ctx));
                return true;
            }

            if (middlewareCallHelper<N + 1, Context, Container, Middlewares...>(middlewares, req, res, ctx)) {
                after_handler_call<CurrentMW, Context, ParentContext_t>(std::get<N>(middlewares), req, res,
                                                                         ctx,
                                                                         static_cast<ParentContext_t &>(ctx));
                return true;
            }

            return false;
        }

        template<int N, typename Context, typename Container>
        bool middlewareCallHelper(Container & /*middlewares*/, server::Request & /*req*/, server::Response & /*res*/,
                                    Context & /*ctx*/) {
            return false;
        }

        template<int N, typename Context, typename Container>
        typename std::enable_if<(N < 0)>::type
        afterHandlersCallHelper(Container & /*middlewares*/, Context & /*context*/, server::Request & /*req*/,
                                   server::Response& /*res*/) {
        }

        template<int N, typename Context, typename Container>
        typename std::enable_if<(N == 0)>::type
        afterHandlersCallHelper(Container &middlewares, Context &ctx, server::Request &req, server::Response &res) {
            using ParentContext_t = typename Context::template Partial<N - 1>;
            using CurrentMW = typename std::tuple_element<N, typename std::remove_reference<Container>::type>::type;
            after_handler_call<CurrentMW, Context, ParentContext_t>(std::get<N>(middlewares), req, res, ctx,
                                                                     static_cast<ParentContext_t &>(ctx));
        }

        template<int N, typename Context, typename Container>
        typename std::enable_if<(N > 0)>::type
        afterHandlersCallHelper(Container &middlewares, Context &ctx, server::Request &req, server::Response &res) {
            using ParentContext_t = typename Context::template Partial<N - 1>;
            using CurrentMW = typename std::tuple_element<N, typename std::remove_reference<Container>::type>::type;
            after_handler_call<CurrentMW, Context, ParentContext_t>(std::get<N>(middlewares), req, res, ctx,
                                                                     static_cast<ParentContext_t &>(ctx));
            afterHandlersCallHelper<N - 1, Context, Container>(middlewares, ctx, req, res);
        }
    }

namespace server {

    define_log_tag(HTTP_CONN);

    class ConnectionImpl : LOGGER(HTTP_CONN) {
    public:
        ConnectionImpl(net::Socket& sock,
                       HttpServerConfig& config,
                       Router& handler,
                       HttpServerStats& stats);

        void start();

        virtual ~ConnectionImpl();

    protected:
        DISABLE_COPY(ConnectionImpl);
        DISABLE_MOVE(ConnectionImpl);

        using SendBuffer = std::vector<net::Chunk>;

        virtual void handleRequest(Request& req, Response& resp) {};

        void sendResponse(Request& req, Response& resp, bool err = false);

        bool writeResponse(const SendBuffer& buf);

        static const char* getCachedDate();

        HttpServerConfig& _config;
        HttpServerStats& _stats;
        net::Socket& _sock;
        Router& _handler;
        Buffer _stage{};
        bool _close{false};
    };

    template<typename... Mws>
    class Connection : public ConnectionImpl {
    public:
        using Middlewares = std::tuple<Mws...>;

        Connection(net::Socket& sock,
                   HttpServerConfig& config,
                   Router& handler,
                   Middlewares* mws,
                   HttpServerStats& stats)
                : ConnectionImpl(sock, config, handler, stats),
                  _mws{mws} {}

    protected:
        void handleRequest(Request& req, Response& resp) override {
            // Populate route parameters before handling request
            Ego._handler.before(req, resp);
            // invoke middleware::before
            crow::Context < Mws...> ctx{};
            req.middlewareContent = &ctx;
            crow::middlewareCallHelper<0, decltype(ctx), decltype(*Ego._mws), Mws...>(
                    *Ego._mws, req, resp, ctx);
            if (!resp.isCompleted()) {
                // invoke route handler
                Ego._handler.handle(req, resp);
            }

            // invoke middleware::after
            crow::afterHandlersCallHelper<(int(sizeof...(Mws))-1), decltype(ctx), decltype(*Ego._mws)>(
                    *Ego._mws, ctx, req, resp);
        }

    private:
        Middlewares* _mws{nullptr};
    };
}

}

#endif //SUIL_HTTP_SERVER_CONNECTION_HPP
