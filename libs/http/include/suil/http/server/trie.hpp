//
// Created by Mpho Mbotho on 2020-12-17.
//

#ifndef SUIL_HTTP_SERVER_TRIE_HPP
#define SUIL_HTTP_SERVER_TRIE_HPP

#include <suil/http/server/rules.hpp>

namespace suil::http::server {

    class RoutesTrie {
    public:
        struct Node {
            unsigned _index{0};
            std::array<unsigned, int(crow::detail::ParamType::MAX)> _paramChildren{};
            UnorderedMap<unsigned> _children{};

            bool isSimple() const;
            bool hasChildren() const;

            ~Node();
        };

        RoutesTrie();

        void validate();

        inline RouterParams find(const String& reqUrl) {
            return Ego.find(reqUrl, head(), 0, nullptr);
        }

        void add(const String& url, unsigned index);
        inline void debug(std::ostream& os) {
            Ego.debug(os, head(), 0);
        }

    private suil_ut:
        void debug(std::ostream& os, Node* node, int level);
        RouterParams find(
                const String& reqUrl,
                const Node* at,
                unsigned  pos,
                crow::detail::routing_params* params);

        void optimize();
        void optimizeNode(Node* node, bool& reworked);
        void gather(std::vector<Node>& dst, Node& node);

        inline const Node* head() const {
            return &Ego._nodes.front();
        }

        inline Node* head() {
            return &Ego._nodes.front();
        }

        unsigned createNode() {
            Ego._nodes.emplace_back(Node{});
            return Ego._nodes.size()-1;
        }

        std::vector<Node> _nodes;
    };
}
#endif //SUIL_HTTP_SERVER_TRIE_HPP
