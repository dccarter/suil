//
// Created by Mpho Mbotho on 2020-12-17.
//

#include "suil/http/server/trie.hpp"

#include <iostream>

namespace suil::http::server {

    bool RoutesTrie::Node::isSimple() const
    {
        if (Ego._index == 0) {
            return std::all_of(std::begin(Ego._paramChildren), std::end(Ego._paramChildren), [](unsigned x) {
                return x == 0;
            });
        }
        return false;
    }

    bool RoutesTrie::Node::hasChildren() const
    {
        return !Ego._children.empty() or
               !std::all_of(std::begin(Ego._paramChildren), std::end(Ego._paramChildren), [](unsigned x) {
                    return x == 0;
                });
    }

    RoutesTrie::Node::~Node()
    {
        Ego._children.clear();
    }

    RoutesTrie::RoutesTrie()
        : _nodes{1}
    {}

    void RoutesTrie::validate()
    {
        if (!Ego.head()->isSimple())
            throw InvalidArguments("Router trie header should be simple!");
        optimize();
    }

    void RoutesTrie::optimizeNode(Node* node, bool& reworked)
    {
        for (auto& x: node->_paramChildren) {
            if (!x) {
                continue;
            }
            optimizeNode(&Ego._nodes[x], reworked);
        }

        if (node->_children.empty()) {
            return;
        }

        bool mergeWithChild{true};
        for (auto& [k, v] : node->_children) {
            if (!Ego._nodes[v].isSimple()) {
                mergeWithChild = false;
                reworked  = true;
                break;
            }
        }

        if (mergeWithChild) {
            decltype(node->_children) merged;
            for (auto& [k, v] : node->_children) {
                auto& child = Ego._nodes[v];
                for (auto& [ck, cv]: child._children) {
                    merged[suil::catstr(k, ck)] = cv;
                }
            }
            node->_children = std::move(merged);
            optimizeNode(node, reworked);
        }
        else {
            for (auto& [k, v] : node->_children) {
                optimizeNode(&Ego._nodes[v], reworked);
            }
        }
    }

    void RoutesTrie::optimize()
    {
        bool reworked{false};
        optimizeNode(head(), reworked);
        decltype(Ego._nodes) tmp;
        tmp.emplace_back();

        for (auto& [k, v]: Ego._nodes.front()._children) {
            auto idx = int(tmp.size());
            gather(tmp, Ego._nodes[v]);
            v = idx;
        }

        for (auto& v: Ego._nodes.front()._paramChildren) {
            if (v == 0) continue;
            auto idx = int(tmp.size());
            gather(tmp, Ego._nodes[v]);
            v = idx;
        }

        tmp.front()._children = std::move(Ego._nodes.front()._children);
        tmp.front()._index = Ego._nodes.front()._index;
        tmp.front()._paramChildren = Ego._nodes.front()._paramChildren;
        Ego._nodes = std::move(tmp);
    }

    void RoutesTrie::gather(std::vector<Node>& dst, Node& node)
    {
        auto at = dst.size();
        dst.emplace_back();
        for (auto& [k, v]: node._children) {
            auto idx = int(dst.size());
            gather(dst, Ego._nodes[v]);
            v = idx;
        }

        for (auto& v: node._paramChildren) {
            if (v == 0) continue;
            auto idx = int(dst.size());
            gather(dst, Ego._nodes[v]);
            v = idx;
        }

        dst[at]._children = std::move(node._children);
        dst[at]._index = node._index;
        dst[at]._paramChildren = node._paramChildren;
    }

    RouterParams RoutesTrie::find(
            const String& reqUrl,
            const Node* node,
            unsigned int pos,
            crow::detail::routing_params* params)
    {
        crow::detail::routing_params empty;
        if (params == nullptr)
            params = &empty;

        unsigned found{};
        crow::detail::routing_params matched;

        if (node == nullptr)
            node = head();
        if (pos == reqUrl.size())
            return {node->_index, std::move(*params)};

        auto updateFound = [&found, &matched](RouterParams& ret)
        {
            if (ret.first && (!found || found > ret.first))
            {
                found = ret.first;
                matched = std::move(ret.second);
            }
        };

        if (node->_paramChildren[int(crow::detail::ParamType::INT)])
        {
            char c = reqUrl[pos];
            if ((c >= '0' && c <= '9') || c == '+' || c == '-')
            {
                char* eptr{nullptr};
                errno = 0;
                auto value = int64(strtoll(&reqUrl[pos], &eptr, 10));
                if (errno != ERANGE && eptr != &reqUrl[pos])
                {
                    params->push(value);
                    auto ret = find(
                            reqUrl,
                            &Ego._nodes[node->_paramChildren[int(crow::detail::ParamType::INT)]],
                            eptr - &reqUrl[0],
                            params);
                    updateFound(ret);
                    params->pop(value);
                }
            }
        }

        if (node->_paramChildren[int(crow::detail::ParamType::UINT)])
        {
            char c = reqUrl[pos];
            if ((c >= '0' && c <= '9') || c == '+')
            {
                char* eptr{nullptr};
                errno = 0;
                auto value = uint64(strtoull(&reqUrl[pos], &eptr, 10));
                if (errno != ERANGE && eptr != &reqUrl[pos])
                {
                    params->push(value);
                    auto ret = find(
                            reqUrl,
                            &Ego._nodes[node->_paramChildren[int(crow::detail::ParamType::UINT)]],
                            eptr - &reqUrl[0],
                            params);
                    updateFound(ret);
                    params->pop(value);
                }
            }
        }

        if (node->_paramChildren[(int)crow::detail::ParamType::DOUBLE])
        {
            char c = reqUrl[pos];
            if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.')
            {
                char* eptr{nullptr};
                errno = 0;
                auto value = strtod(&reqUrl[pos], &eptr);
                if (errno != ERANGE && eptr != &reqUrl[pos])
                {
                    params->push(value);
                    auto ret = find(
                            reqUrl,
                            &Ego._nodes[node->_paramChildren[int(crow::detail::ParamType::DOUBLE)]],
                            eptr - &reqUrl[0],
                            params);
                    updateFound(ret);
                    params->pop(value);
                }
            }
        }

        if (node->_paramChildren[int(crow::detail::ParamType::STRING)])
        {
            size_t epos = pos;
            for(; epos < reqUrl.size(); epos++)
            {
                if (reqUrl[epos] == '/')
                    break;
            }

            if (epos != pos)
            {
                params->push(reqUrl.substr(pos, epos-pos));
                auto ret = find(
                        reqUrl,
                        &Ego._nodes[node->_paramChildren[int(crow::detail::ParamType::STRING)]],
                        epos,
                        params);
                updateFound(ret);
                params->pop(String{});
            }
        }

        if (node->_paramChildren[int(crow::detail::ParamType::PATH)])
        {
            size_t epos = reqUrl.size();

            if (epos != pos)
            {
                params->push(reqUrl.substr(pos, epos-pos));
                auto ret = find(
                        reqUrl,
                        &Ego._nodes[node->_paramChildren[int(crow::detail::ParamType::PATH)]],
                        epos,
                        params);
                updateFound(ret);
                params->pop(String{});
            }
        }

        for(const auto& [fragment, index] : node->_children)
        {
            const auto& child = &Ego._nodes[index];

            if (reqUrl.substr(pos).startsWith(fragment))
            {
                auto ret = find(reqUrl, child, pos + fragment.size(), params);
                updateFound(ret);
            }
        }

        return {found, std::move(matched)};
    }

    void RoutesTrie::add(const String& url, unsigned int index)
    {
        unsigned idx{0};

        for(unsigned i = 0; i < url.size(); i ++)
        {
            char c = url[i];
            if (c == '{')
            {
                static struct ParamTraits
                {
                    crow::detail::ParamType type;
                    String name;
                } paramTraits[] = {
                    { crow::detail::ParamType::INT, "{int}" },
                    { crow::detail::ParamType::UINT, "{uint}" },
                    { crow::detail::ParamType::DOUBLE, "{float}" },
                    { crow::detail::ParamType::DOUBLE, "{double}" },
                    { crow::detail::ParamType::STRING, "{str}" },
                    { crow::detail::ParamType::STRING, "{string}" },
                    { crow::detail::ParamType::PATH, "{path}" },
                };
                auto ii{i};
                for(auto& x:paramTraits)
                {
                    auto tmp = url.substr(i, x.name.size());
                    if (tmp == x.name)
                    {
                        if (!Ego._nodes[idx]._paramChildren[(int)x.type])
                        {
                            auto nidx = createNode();
                            Ego._nodes[idx]._paramChildren[(int)x.type] = nidx;
                        }
                        idx = Ego._nodes[idx]._paramChildren[(int)x.type];
                        ii += x.name.size();
                        break;
                    }
                }
                if (i != ii) {
                    i = ii-1;
                }
            }
            else
            {
                String piece(&c, 1, false);
                if (!Ego._nodes[idx]._children.contains(piece))
                {
                    auto nidx = createNode();
                    Ego._nodes[idx]._children.emplace(piece.dup(), nidx);
                }
                idx = Ego._nodes[idx]._children[piece];
            }
        }
        if (Ego._nodes[idx]._index)
            throw InvalidArguments("router rule '", url, "' already registered");
        Ego._nodes[idx]._index = index;
    }

    void RoutesTrie::debug(std::ostream& os, Node* node, int level)
    {
        for (int i = 0; i < int(crow::detail::ParamType::MAX); i++) {
            if (node->_paramChildren[i] != 0) {
                switch (crow::detail::ParamType(i)) {
                    case crow::detail::ParamType::INT:
                        os << "{int}";
                        break;
                    case crow::detail::ParamType::UINT:
                        os << "{uint}";
                        break;
                    case crow::detail::ParamType::DOUBLE:
                        os << "{float}";
                        break;
                    case crow::detail::ParamType::STRING:
                        os << "{str}";
                        break;
                    case crow::detail::ParamType::PATH:
                        os << "<path>";
                        break;
                    default:
                        os << "{ERROR}";
                        break;
                }

                debug(os, &Ego._nodes[node->_paramChildren[i]], 0);
                if(Ego._nodes[node->_paramChildren[i]]._children.empty())
                    os << "\n";
            }
        }

        for (auto& [k, v]: node->_children) {
            os << std::string(size_t(2*level), ' ');
            os << k;
            auto& n = Ego._nodes[v];
            if (n.hasChildren()) {
                debug(os, &n, level + 1);
            }
            else {
                std::cout << "\n";
            }
        }
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

using suil::http::server::RoutesTrie;
using suil::String;
namespace crow = suil::http::crow;

TEST_CASE("RoutesTrie API's", "[http][http-server][routes-trie]")
{
    WHEN("Adding routes to a trie") {
        int index{0};
        RoutesTrie trie;
        trie.add("/", index++);
        REQUIRE(trie._nodes.size() == 2);
        trie.add("/users", index++);
        REQUIRE(trie._nodes.size() == 7);
        int i = 0;
        for (auto c: std::string{"/users"}) {
            auto& n = trie._nodes[i++];
            REQUIRE(n._children.size() == 1);
            REQUIRE(n._children.contains(String{&c, 1, false}));
        }

        trie.add("/users/{int}", index++);
        REQUIRE(trie._nodes.size() == 9);
        auto& n = trie._nodes[7];
        REQUIRE(n._children.empty());
        REQUIRE(n._paramChildren[0] == 8);
        trie.add("/admin/{string}", index++);
        REQUIRE(trie._nodes.size() == 16);
        REQUIRE(trie._nodes[0]._children.size() == 1);
        REQUIRE(trie._nodes[1]._children.size() == 2);
        REQUIRE_THROWS(trie.add("/admin/{string}", index));
    }

    WHEN("Finding a route in a trie") {
        int index{1};
        RoutesTrie trie;
        trie.add("/", index++);
        trie.add("/users/list", index++);
        trie.add("/users/get/{int}", index++);
        trie.add("/users/set/{int}/{string}", index++);
        trie.add("/temps/set/{string}/and/{path}", index++);
        trie.add("/admin/add/{int}/{double}", index++);
        trie.optimize();
        auto params = trie.find("/users/get/100");
        REQUIRE(params.first == 3);
        REQUIRE(params.second.int_params.back == 1);
        REQUIRE(params.second.int_params[0] == 100);

        params = trie.find("/users/set/100/Carter");
        REQUIRE(params.first == 4);
        REQUIRE(params.second.int_params.back == 1);
        REQUIRE(params.second.int_params[0] == 100);
        REQUIRE(params.second.string_params.back == 1);
        REQUIRE(params.second.string_params[0].str == "Carter");

        params = trie.find("/temps/set/home/and//home/carter");
        REQUIRE(params.first == 5);
        REQUIRE(params.second.string_params.back == 2);
        REQUIRE(params.second.string_params[0].str == "home");
        REQUIRE(params.second.string_params[1].str == "/home/carter");
    }
}
#endif