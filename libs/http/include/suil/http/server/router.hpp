//
// Created by Mpho Mbotho on 2020-12-18.
//

#ifndef SUIL_HTTP_SERVER_ROUTER_HPP
#define SUIL_HTTP_SERVER_ROUTER_HPP

#include <suil/http/server/trie.hpp>

namespace suil::http::server {

    define_log_tag(HTTP_ROUTER);

    class Router: LOGGER(HTTP_ROUTER) {
    public:
        static constexpr int RuleSpecialRedirectSlash = 1;
        using Enumerator = std::function<bool(BaseRule&)>;
        using ConstEnumerator = std::function<bool(const BaseRule&)>;
        explicit Router(String base);

        MOVE_CTOR(Router) = default;
        MOVE_ASSIGN(Router) = default;

        DynamicRule& createDynamicRule(const String& rule);

        template <uint64 N>
        typename crow::magic::arguments<N>::type::template rebind<TaggedRule>&
        createTaggedRule(const String& rule) {
            using Rule = typename crow::magic::arguments<N>::type::template rebind<TaggedRule>;
            auto taggedRule = new Rule(rule.dup());
            internalAddRule(rule, taggedRule);
            return *taggedRule;
        }

        void validate();

        void handle(Request& req, Response& resp);

        Router& operator|(Enumerator enumerator);
        const Router& operator|(ConstEnumerator enumerator) const;

        void toString(std::ostream& os);

        inline const String& apiBase() const {
            return Ego._apiBase;
        }

    private suil_ut:
        DISABLE_COPY(Router);

        template <typename ...T>
        friend class Connection;

        void before(Request& req, Response& resp);
        void internalAddRule(const String& rule, BaseRule* obj);
        std::vector<std::unique_ptr<BaseRule>> _rules{2};
        String      _apiBase{""};
        RoutesTrie _trie{};
    };
}
#endif //SUIL_HTTP_SERVER_ROUTER_HPP
