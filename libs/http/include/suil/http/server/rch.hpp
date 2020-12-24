//
// Created by Mpho Mbotho on 2020-12-17.
//

#ifndef SUIL_HTTP_SERVER_RCH_HPP
#define SUIL_HTTP_SERVER_RCH_HPP

#include <suil/http/server/request.hpp>
#include <suil/http/server/response.hpp>

#include <suil/http/server/crow.hpp>

namespace suil::http::crow {

    namespace rch {
        template<typename T, int Pos>
        struct call_pair {
            using type = T;
            static const int pos = Pos;
        };

        template<typename H1>
        struct call_params {
            H1& handler;
            const crow::detail::routing_params& params;
            const server::Request& req;
            server::Response& res;
        };

        template<typename F, int NInt, int NUint, int NDouble, int NString, typename S1, typename S2>
        struct call {
        };

        template<typename F, int NInt, int NUint, int NDouble, int NString, typename ... Args1, typename ... Args2>
        struct call<F, NInt, NUint, NDouble, NString, magic::S<int64_t, Args1...>, magic::S<Args2...>> {
            void operator()(F cparams) {
                using pushed = typename magic::S<Args2...>::template push_back<call_pair<int64_t, NInt>>;
                call<F, NInt + 1, NUint, NDouble, NString,
                        magic::S<Args1...>, pushed>()(cparams);
            }
        };

        template<typename F, int NInt, int NUint, int NDouble, int NString, typename ... Args1, typename ... Args2>
        struct call<F, NInt, NUint, NDouble, NString, magic::S<uint64_t, Args1...>, magic::S<Args2...>> {
            void operator()(F cparams) {
                using pushed = typename magic::S<Args2...>::template push_back<call_pair<uint64_t, NUint>>;
                call<F, NInt, NUint + 1, NDouble, NString,
                        magic::S<Args1...>, pushed>()(cparams);
            }
        };

        template<typename F, int NInt, int NUint, int NDouble, int NString, typename ... Args1, typename ... Args2>
        struct call<F, NInt, NUint, NDouble, NString, magic::S<double, Args1...>, magic::S<Args2...>> {
            void operator()(F cparams) {
                using pushed = typename magic::S<Args2...>::template push_back<call_pair<double, NDouble>>;
                call<F, NInt, NUint, NDouble + 1, NString,
                        magic::S<Args1...>, pushed>()(cparams);
            }
        };

        template<typename F, int NInt, int NUint, int NDouble, int NString, typename ... Args1, typename ... Args2>
        struct call<F, NInt, NUint, NDouble, NString, magic::S<String, Args1...>, magic::S<Args2...>> {
            void operator()(F cparams) {
                using pushed = typename magic::S<Args2...>::template push_back<call_pair<String, NString>>;
                call<F, NInt, NUint, NDouble, NString + 1,
                        magic::S<Args1...>, pushed>()(cparams);
            }
        };

        template<typename F, int NInt, int NUint, int NDouble, int NString, typename ... Args1>
        struct call<F, NInt, NUint, NDouble, NString, magic::S<>, magic::S<Args1...>> {
            void operator()(F cparams) {
                cparams.handler(
                        cparams.req,
                        cparams.res,
                        cparams.params.template get<typename Args1::type>(Args1::pos)...);
            }
        };

        template<typename Func, typename ... ArgsWrapped>
        struct Wrapped {
            template<typename ... Args>
                requires (!std::is_same_v<std::tuple_element_t<0, std::tuple<Args..., void>>, const server::Request&>)
            void set(Func f) {
                handler_ = ([f = std::move(f)](const server::Request&, server::Response& res, Args... args) {
                    res = server::Response(f(args...));
                    res.end();
                });
            }

            template<typename Req, typename ... Args>
            struct req_handler_wrapper {
                req_handler_wrapper(Func f)
                        : f(std::move(f)) {
                }

                void operator()(const server::Request& req, server::Response& res, Args... args) {
                    res = server::Response(f(req, args...));
                    res.end();
                }

                Func f;
            };

            template<typename ... Args>
            requires (std::is_same_v<std::tuple_element_t<0, std::tuple<Args..., void>>, const server::Request&> and
                      !std::is_same_v<std::tuple_element_t<1, std::tuple<Args..., void, void>>, server::Response&>)
            void set(Func f) {
                handler_ = req_handler_wrapper<Args...>(std::move(f));
            }

            template<typename ... Args>
            requires (std::is_same_v<std::tuple_element_t<0, std::tuple<Args..., void>>, const server::Request&> and
                      std::is_same_v<std::tuple_element_t<1, std::tuple<Args..., void, void>>, server::Response&>)
            void set(Func f) {
                handler_ = std::move(f);
            }

            template<typename ... Args>
            struct handler_type_helper {
                using type = std::function<void(const server::Request&, server::Response&, Args...)>;
                using args_type = magic::S<typename magic::promote_t<Args>...>;
            };

            template<typename ... Args>
            struct handler_type_helper<const server::Request&, Args...> {
                using type = std::function<void(const server::Request&, server::Response&, Args...)>;
                using args_type = magic::S<typename magic::promote_t<Args>...>;
            };

            template<typename ... Args>
            struct handler_type_helper<const server::Request&, server::Response&, Args...> {
                using type = std::function<void(const server::Request&, server::Response&, Args...)>;
                using args_type = magic::S<typename magic::promote_t<Args>...>;
            };

            typename handler_type_helper<ArgsWrapped...>::type handler_;

            void operator()(const server::Request& req, server::Response& res, const crow::detail::routing_params& params) {
                using Invoker = rch::call<rch::call_params<decltype(handler_)>,
                        0,
                        0,
                        0,
                        0,
                        typename handler_type_helper<ArgsWrapped...>::args_type,
                        magic::S<>
                >;

                Invoker()(rch::call_params<decltype(handler_)>{handler_, params, req, res});
            }
        };
    }

}

#endif //SUIL_HTTP_SERVER_RCH_HPP
