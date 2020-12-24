//
// Created by Mpho Mbotho on 2020-12-17.
//

#include "suil/http/server/rules.hpp"

namespace suil::http::server {

    uint32 BaseRule::sIdGenerator{0};

    BaseRule::BaseRule(String rule)
        : _rule{std::move(rule)}
    {
        Ego._id = sIdGenerator;
        sIdGenerator++;
    }

    RouteSchema BaseRule::schema() const
    {
        RouteSchema sc;
        schema(sc);
        return sc;
    }

    void BaseRule::schema(RouteSchema& dst) const
    {
        dst.Name = Ego._name.peek();
        dst.Rule = Ego._rule.peek();
        dst.Id = Ego._id;
        dst.Description = Ego._description;
        dst.Attrs.Static =       Ego._attrs.Static;
        dst.Attrs.Authorize =    Ego._attrs.Authorize;
        dst.Attrs.ParseCookies = Ego._attrs.ParseCookies;
        dst.Attrs.ParseForm =    Ego._attrs.ParseForm;
        dst.Attrs.ReplyType =    Ego._attrs.ReplyType;
        dst.Attrs.Enabled =      Ego._attrs.Enabled;
        for (auto& m: Ego._methodNames) {
            dst.Methods.emplace_back(m.peek());
        }
    }

    void DynamicRule::validate()
    {
        if (Ego._erasedHandler == nullptr) {
            throw InvalidArguments(Ego._name,
                                   (Ego._name.empty()? "" : ": "),
                                   "no handler for URL ", Ego._rule);
        }
    }

    void DynamicRule::handle(
            const Request& req,
            Response& resp,
            const crow::detail::routing_params& params)
    {
        Ego._erasedHandler(req, resp, params);
    }
}