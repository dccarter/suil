//
// Created by Mpho Mbotho on 2020-12-17.
//

#ifndef SUIL_HTTP_SERVER_RULES_HPP
#define SUIL_HTTP_SERVER_RULES_HPP

#include <suil/http/common.hpp>
#include <suil/http/server/rch.hpp>

#include <suil/base/string.hpp>

namespace suil::http::server {

    struct Desc {
        explicit Desc(String str)
            : desc{std::move(str)}
        {}

        MOVE_CTOR(Desc) = default;
        MOVE_ASSIGN(Desc) = default;

        DISABLE_COPY(Desc);

        String desc{};
    };

    class BaseRule {
    public:
        BaseRule(String rule);

        virtual ~BaseRule() = default;

        virtual void validate() = 0;
        virtual void handle(
                const Request& req,
                Response& resp,
                const crow::detail::routing_params& params) = 0;
        inline uint32 getMethods() const {
            return Ego._methods;
        }

        inline uint32 id() const {
            return Ego._id;
        }

        inline const String& name() const {
            return Ego._name;
        }

        inline const String& description() const {
            return Ego._description;
        }

        inline const String& path() const {
            return Ego._rule;
        }

        RouteSchema schema() const;

        void schema(RouteSchema& dst) const;

        RouteAttributes _attrs{};
    protected:
        uint32 _methods{1 << int(Method::Get)};
        String _name{};
        String _rule{};
        String _description{};
        std::vector<String> _methodNames{};
        uint32 _id{0};

        template <typename T>
        friend struct RuleParameterTraits;
        static uint32 sIdGenerator;
    };

    template <typename T>
    struct RuleParameterTraits {
        using Self = T;

        Self& name(String name) noexcept {
            ((Self *) this)->_name = std::move(name);
            return (Self &) Ego;
        }

        Self& method(Method m) {
            ((Self *) this)->_methods |= 1 << int(m);
            ((Self *) this)->_methodNames.emplace_back(toString(m));
            return (Self &) Ego;
        }

        template <typename... Methods>
        Self& methods(Method m, const Methods&... methods) {
            ((Self *) this)->method(m);
            if constexpr (sizeof...(methods) > 0) {
                Ego.template methods(methods...);
            }
            return (Self &) Ego;
        }

        Self& describe(String desc)
        {
            ((Self *) this)->_description = std::move(desc);
            return (Self &) Ego;
        }
    };

    class DynamicRule: public BaseRule, public RuleParameterTraits<DynamicRule> {
    public:
        using ErasedHandler = std::function<void(const Request&, Response&, const crow::detail::routing_params&)>;
        DynamicRule(String rule)
            : BaseRule(std::move(rule))
        {}

        void validate() override;

        void handle(
                const Request &req,
                Response &resp,
                const crow::detail::routing_params &params) override;

        DynamicRule& operator()(Method m) {
            return Ego.methods(m);
        }

        template <typename... Methods>
        DynamicRule& operator()(Method m, Methods... ms) {
            return Ego.methods(m, ms...);
        }

        DynamicRule& operator()(Desc desc) {
            return Ego.describe(std::move(desc.desc));
        }

        template <typename ...Attrs>
        DynamicRule& attrs(Attrs&&... opts) {
            suil::applyConfig(Ego._attrs, std::move(opts)...);
            return Ego;
        }

        template <typename Func>
        uint32 operator()(Func func) {
            using Function = crow::function_traits<Func>;
            Ego._erasedHandler = wrap(std::move(func), crow::magic::gen_seq<Function::arity>());
            return Ego._id;
        }

        template <typename Func, unsigned ...Indices>
        ErasedHandler wrap(Func f, crow::magic::seq<Indices...>)
        {
            using Function = crow::function_traits<Func>;
            constexpr auto tag = crow::magic::compute_parameter_tag_from_args_list<typename Function::template arg<Indices>...>::value;
            if (!crow::magic::is_parameter_tag_compatible(
                    crow::magic::get_parameter_tag_runtime(Ego._rule()), tag))
            {
                throw InvalidArguments("DynamicRule '", Ego._rule, "' handler mis-matches rule parameters");
            }
            using Helper = crow::rch::Wrapped<Func, typename Function::template arg<Indices>...>;
            auto ret = Helper();
            ret.template set<typename Function::template arg<Indices>...>(std::move(f));
            return ret;
        }

        template <typename Func>
        void operator()(String name, Func&& func) {
            Ego._name = std::move(name);
            Ego.template operator()<Func>(std::forward<Func>(func));
        }

    private:
        ErasedHandler _erasedHandler{nullptr};
    };

    template <typename ...Args>
    class TaggedRule: public BaseRule, public RuleParameterTraits<TaggedRule<Args...>> {
    public:
        using Self = TaggedRule<Args...>;
        using Handler = std::function<void(const Request&, Response&, Args...)>;

        TaggedRule(String rule)
            : BaseRule(std::move(rule))
        {}

        void validate() override {
            if (Ego._handler == nullptr) {
                throw InvalidArguments(Ego._name,
                                       (Ego._name.empty()? "" : ": "),
                                       "no handler for URL ", Ego._rule);
            }
        }

        Self& operator()(Method m) {
            return Ego.methods(m);
        }

        template<typename... Methods>
        Self &operator()(Method m, Methods... ms) {
            return Ego.methods(m, ms...);
        }

        template <typename... Opts>
        Self& attrs(Opts&&... opts) {
            /* apply configuration options */
            suil::applyConfig(Ego._attrs, std::forward<Opts>(opts)...);
            return *this;
        }

        Self& operator()(Desc desc) {
            return Ego.describe(std::move(desc.desc));
        }

        template <typename Func>
            requires crow::magic::CallHelper<Func, crow::magic::S<Args...>>::value
        uint32 operator()(Func&& func) {
            static_assert((crow::magic::CallHelper<Func, crow::magic::S<Args...>>::value or
                          crow::magic::CallHelper<Func, crow::magic::S<Request, Args...>>::value),
                         "Handler type is mismatched with URL parameters");
            static_assert(!std::is_same<void, decltype(func(std::declval<Args>()...))>::value,
                          "Handler function cannot have void return type, see http::Response::Response for valid return types");

            Ego._handler = [f = std::forward<Func>(func)](const Request&, Response& res, Args... args) {
                res.merge(Response(f(args...)));
                res.end();
            };
            return Ego._id;
        }

        template <typename Func>
        requires (!crow::magic::CallHelper<Func, crow::magic::S<Args...>>::value and
                   crow::magic::CallHelper<Func, crow::magic::S<Request, Args...>>::value)
        uint32 operator()(Func&& func) {
            static_assert((crow::magic::CallHelper<Func, crow::magic::S<Args...>>::value or
                           crow::magic::CallHelper<Func, crow::magic::S<Request, Args...>>::value),
                          "Handler type is mismatched with URL parameters");
            static_assert(!std::is_same<void, decltype(func(std::declval<Request>(), std::declval<Args>()...))>::value,
                          "Handler function cannot have void return type, see http::Response::Response for valid return types");

            Ego._handler = [f = std::forward(func)](const Request& req, Response& res, Args... args) {
                res.merge(Response(f(req,  args...)));
                res.end();
            };
            return Ego._id;
        }

        template <typename Func>
        requires (!crow::magic::CallHelper<Func, crow::magic::S<Args...>>::value and
                  !crow::magic::CallHelper<Func, crow::magic::S<Request, Args...>>::value)
        uint32 operator()(Func&& func) {
            static_assert((crow::magic::CallHelper<Func, crow::magic::S<Args...>>::value or
                           crow::magic::CallHelper<Func, crow::magic::S<Request, Args...>>::value or
                           crow::magic::CallHelper<Func, crow::magic::S<Request, Response&, Args...>>::value),
                          "Handler type is mismatched with URL parameters");
            static_assert(std::is_same<void, decltype(func(std::declval<Request>(), std::declval<Response &>(),
                                                        std::declval<Args>()...))>::value,
                          "Handler function with http::Response argument must always return void");

            Ego._handler = std::forward<Func>(func);
            return Ego._id;
        }

        template <typename Func>
        void operator()(String name, Func&& func) {
            Ego._name = std::move(name);
            Ego.template operator()<Func>(std::forward(func));
        }

        void handle(
                const Request &req,
                Response &resp,
                const crow::detail::routing_params &params) override
        {
            using CallParams = crow::rch::call_params<decltype(Ego._handler)>;
            using Helper = crow::rch::call<CallParams,
                                0, 0, 0, 0,
                                crow::magic::S<Args...>,
                                crow::magic::S<>>;
            Helper{}(CallParams{Ego._handler, params, req, resp});
        }

    private:
        Handler _handler{nullptr};
    };
}
#endif //SUIL_HTTP_SERVER_RULES_HPP
