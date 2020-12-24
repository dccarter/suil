//
// Created by Mpho Mbotho on 2020-12-18.
//

#include "suil/http/server/router.hpp"

#ifndef SUIL_FILE_SERVER_ROUTE
#define SUIL_FILE_SERVER_ROUTE "1b7647618ea63937daae8ebb903e9175"
#endif

namespace suil::http::server {

    Router::Router(String base)
        : _apiBase{(base == "/")? "": std::move(base)}
    {}

    void Router::toString(std::ostream& os)
    {
        Ego._trie.debug(os);
    }

    DynamicRule& Router::createDynamicRule(const String& rule)
    {
        auto obj = new DynamicRule(rule.dup());
        internalAddRule(rule, obj);
        return *obj;
    }

    void Router::validate()
    {
        Ego._trie.validate();
        for (auto& rule: Ego._rules) {
            if (rule) {
                rule->validate();
            }
        }
    }

    void Router::internalAddRule(const String& rule, BaseRule* obj)
    {
        Ego._rules.emplace_back(obj);
        Ego._trie.add(rule, Ego._rules.size()-1);

        if (rule.back() == '/') {
            auto withoutTrailingSlash = rule.substr(0, rule.size()-1);
            Ego._trie.add(withoutTrailingSlash, RuleSpecialRedirectSlash);
        }
    }

    void Router::before(Request& req, Response& resp)
    {
        String url{};
        auto tmp = req.url().peek();
        if (tmp.empty() or tmp == "/") {
            url = "/";
        }

        if (Ego._apiBase.empty() or tmp.startsWith(Ego._apiBase)) {
            // this request is targeting an API rule
            url = tmp.substr(Ego._apiBase.size());
        }
        else {
            // assume request is targeting a file server
            url = "/" SUIL_FILE_SERVER_ROUTE;
        }

        auto params = Ego._trie.find(url);
        if (params.first == 0) {
            // route does not exist
            throw HttpError(http::NotFound);
        }

        if (params.first >= Ego._rules.size()) {
            throw HttpError(
                    http::InternalError,
                    "System error, contact system administrator");
        }

        auto& rule = *Ego._rules[params.first];
        if (!rule._attrs.Enabled) {
            throw HttpError(http::NotFound);
        }

        // update request with parameters
        req._params.index = params.first;
        req._params.decoded = std::move(params.second);
        req._params.attrs = &rule._attrs;
        req._params.methods =  rule.getMethods();
        req._params.routeId = rule.id();
    }

    void Router::handle(Request& req, Response& resp)
    {
        if (req._params.index == RuleSpecialRedirectSlash) {
            resp = Response{};
            if (const auto& host = req.header("Host")) {
                resp.header("Location", suil::catstr("http://", host, req.url(), "/"));
            }
            else {
                resp.header("Location", suil::catstr( req.url(), "/"));
            }
            resp.end(http::MovedPermanently);
            return;
        }

        auto& rule = *Ego._rules[req._params.index];
        if (0 == (rule.getMethods() & (1 << uint32(req.method)))) {
            throw HttpError(http::NotFound);
        }

        try {
            rule.handle(req, resp, req._params.decoded);
        }
        catch (HttpError& ex) {
            throw;
        }
        catch(...) {
            auto ex = Exception::fromCurrent();
            idebug("route(%s) exception: %s", rule.name()(), ex.what());
            throw HttpError(http::InternalError);
        }
    }

    Router& Router::operator|(Enumerator enumerator)
    {
        for (auto& rule: Ego._rules) {
            if (unlikely(rule == nullptr)) {
                continue;
            }
            else {
                if (!enumerator(*rule)) {
                    // enumeration terminated
                    break;
                }
            }
        }

        return Ego;
    }

    const Router& Router::operator|(ConstEnumerator enumerator) const
    {
        for (auto& rule: Ego._rules) {
            if (unlikely(rule == nullptr)) {
                continue;
            }
            else {
                if (!enumerator(*rule)) {
                    // enumeration terminated
                    break;
                }
            }
        }

        return Ego;
    }
}