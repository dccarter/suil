//
// Created by Mpho Mbotho on 2020-10-16.
//

#include "suil/base/mustache.hpp"
#include "suil/base/file.hpp"

namespace suil {

    Mustache::Mustache(suil::Mustache &&o) noexcept
            : mFragments(std::move(o.mFragments)),
              mActions(std::move(o.mActions)),
              mBody(o.mBody)
    {}

    Mustache& Mustache::operator=(suil::Mustache &&o) noexcept{
        if (this != &o) {
            mFragments = std::move(o.mFragments);
            mActions = std::move(o.mActions);
            mBody = std::move(o.mBody);
        }
        return Ego;
    }

    void Mustache::readTag(Parser& p, BlockPositions& blocks) {
        // read parser tag
        p.eatSpace();
        auto tok = p.peek();

        auto toLastIndex = [](Parser& pp, const char* chs = "_") -> std::pair<size_t,size_t> {
            pp.eatSpace();
            size_t s = pp.pos, e = pp.pos;
            char n{pp.next()};
            if (!isalpha(n) && n != '_') {
                // implementation supports tags that starts with alpha/_
                throw MustacheParseError("tags can only start with letters or _, at position '",
                                        s, ", ^^^", pp.substr(s-3, std::min(size_t(10), pp.size-s)));
            }

            while(!pp.eof() && (isalnum(n = pp.peek()) || strchr(chs, n) != nullptr))
                pp.next();
            e = pp.pos;
            pp.eatSpace();
            if (pp.next() != '}' || pp.next() != '}') {
                // error expected '}}'
                throw MustacheParseError("unclosed block close tag at position '",
                                        s-2, "', ^^^", pp.substr(s, pp.pos));
            }
            if (e == s) {
                // error empty block
                throw MustacheParseError("empty tags not supported at position '", s-2, "' ^^^",
                                        pp.substr(s-2, pp.pos));
            }

            return std::make_pair(s, e);
        };

        switch (tok) {
            case '#': {
                p.next();
                auto[start, last] = toLastIndex(p);
                blocks.emplace_back(mActions.size());
                mActions.push_back(Action{OpenBlock, start, last, 0});
                p.frag = p.pos;
                break;
            }
            case '/': {
                if (blocks.empty()) {
                    // unexpected close tag
                    throw MustacheParseError("unexpected close tag at position '",
                                            p.pos, "', ^^^",
                                            p.substr(p.pos-1,std::min(size_t(10), p.size-p.pos)));
                }
                auto openTag = p.tagName(mActions[blocks.back()]);
                p.next();
                auto [start, last] = toLastIndex(p);
                Action action{CloseBlock, start, last, blocks.back()};
                auto closeTag = p.tagName(action);
                if (openTag != closeTag) {
                    // unexpected close tag,
                    throw MustacheParseError("unexpected block close tag at position '",
                                            start, "', got '", closeTag, "', expecting '",
                                            openTag, "'");
                }
                mActions.push_back(action);
                blocks.pop_back();
                p.frag = p.pos;
                break;
            }
            case '^': /* ELSE */{
                p.next();
                auto [start, last] = toLastIndex(p);
                blocks.emplace_back(mActions.size());
                mActions.push_back(Action{ElseBlock, start, last, 0});
                p.frag = p.pos;
                break;
            }
            case '!': /* DO NOTHING */ {
                p.next();
                size_t start{p.pos}, end{p.pos};
                while (!p.eof() && p.peek() != '}' && p.peek(1) != '}')
                    p.next();
                if (p.next() != '}' && p.peek() != '}') {
                    // comment not closed
                    throw MustacheParseError("ignore tag not closed at position '",
                                            start, "', ^^^",
                                            p.substr(start, std::min(size_t(20), p.pos-start)));
                }
                mActions.push_back(Action{Ignore, start, p.pos, 0});
                p.next();p.next();
                p.frag = p.pos;
                break;
            }

            case '>': /* PARTIAL */ {
                p.next();
                auto [start, last] = toLastIndex(p, "/._");
                mActions.push_back(Action{Partial, start, last, 0});
                p.frag = p.pos;
                break;
            }

            case '{': /* UN ESCAPE */ {
                p.next();
                auto [start, last] = toLastIndex(p);
                tok = p.next();
                if (tok != '}') {
                    // un-matched un-escape tag
                    throw MustacheParseError("unmatched un-escape tag at position '",
                                            start, "', ^^^", p.substr(start-2, p.pos));
                }
                mActions.push_back(Action{UnEscapeTag, start, last, 0});
                p.frag = p.pos;
                break;
            }

            case '&': /* UN ESCAPE */ {
                p.next();
                auto [start, last] = toLastIndex(p);
                mActions.push_back(Action{UnEscapeTag, start, last, 0});
                p.frag = p.pos;
                break;
            }

            case '=': /* CHANGE DELIMITER */ {
                throw MustacheParseError(
                        "changing delimiter currently not supported, at positon '", p.pos, "'");
                break;
            }

            default: /* NORMAL TAG */{
                auto [start, last] = toLastIndex(p);
                mActions.push_back(Action{Tag, start, last, 0});
                p.frag = p.pos;
                break;
            }
        }
    }

    void Mustache::parse()
    {
        if (mBody.empty()) {
            // cannot parse empty body
            idebug("cannot parse an empty mustache template");
            return;
        }

        struct Parser p(mBody);
        BlockPositions blocks;
        while (!p.eof()) {
            // Parse until end
            char tok = p.next(), next = p.peek();
            if (tok == '{' && next == '{') {
                // opening fragment
                mFragments.emplace_back(p.frag, p.pos-1);
                p.next();
                // read body tag
                readTag(p, blocks);
            }
        }

        if (!blocks.empty()) {
            // un-terminated blocks
            std::stringstream ss;
            ss << "template contains non-terminated blocks";
            for (auto &b : blocks)
                ss << ", '" << p.tagName(mActions[b]) << "'";
            throw MustacheParseError(ss.str());
        }

        if (p.pos != p.frag) {
            // append last fragment
            mFragments.emplace_back(p.frag, p.pos);
        }

        if (!mActions.empty()) {
            for (int i = 0; i < mActions.size() - 1; i++) {
                const auto &b = mActions[i];
                if (b.tag == ElseBlock)
                    if (mActions[i + 1].tag != CloseBlock)
                        throw MustacheParseError("inverted section cannot have tags {",
                                                tagView(b), " ", b.first, "}");

            }
        }
    }

    void Mustache::dump()
    {
        Buffer ob{mBody.size()*2};
        for (size_t i = 0; i < mFragments.size(); i++) {
            auto Frag = mFragments[i];
            auto frag = fragString(Frag);
            ob << "FRAG: ";
            if (!frag.empty())
                ob << frag << "\n";
            else
                ob << "{" << Frag.first << ", " << Frag.second << "}\n";
            if (i < mActions.size()) {
                // print action
                auto act = tagString(mActions[i]);
                ob << "\tACTION: " << act << "\n";
            }
        }

        (void)write(STDOUT_FILENO, ob.data(), ob.size());
        syncfs(STDOUT_FILENO);
    }

    void Mustache::escape(Buffer& out, const String& in) const
    {
        out.reserve(in.size()*2);
        for (auto c : in) {
            // escape every character of input
            switch (c) {
                case '&' : out << "&amp;"; break;
                case '<' : out << "&lt;"; break;
                case '>' : out << "&gt;"; break;
                case '"' : out << "&quot;"; break;
                case '\'' : out << "&#39;"; break;
                case '/' : out << "&#x2f;"; break;
                default:
                    out.append(&c, 1);
            }
        }
    }

    void Mustache::renderFragment(Buffer& out, const Mustache::Fragment& frag) const
    {
        if (!isFragEmpty(frag)) {
            // only render non-empty fragments
            out << fragString(frag);
        }
    }

    void Mustache::renderTag(Buffer& out, const Action& act, const json::Object& ctx, int& index) const
    {
        auto name = tagString(act);

        switch (act.tag) {
            case Ignore:
                /* ignore comments*/
                break;
            case Partial: {
                /* load a and render template */
                const Mustache& m = MustacheCache::get().load(tagString(act));
                m.render(out, ctx);
                break;
            }
            case UnEscapeTag:
            case Tag: {
                /* render normal tag */
                auto val = ctx[name.peek()];
                switch(val.type()) {
                    /* render base on type */
                    case json::Tag::JSON_BOOL: {
                        /* render boolean */
                        out << (((bool) val)? "true" : "false");
                        break;
                    }
                    case json::Tag::JSON_NUMBER: {
                        /* render number */
                        auto d = (double) val;
                        if (d != (int) d)
                            out << d;
                        else
                            out << (int) d;
                        break;
                    }
                    case json::Tag::JSON_STRING: {
                        /* render string */
                        if (act.tag == UnEscapeTag)
                            out << (String) val;
                        else
                            escape(out, val);
                        break;
                    }
                    default:
                        /* rendering failure */
                        throw MustacheParseError(
                                "rendering template tag {", tagView(act), "@", act.pos, "}");
                }
                break;
            }

            case ElseBlock: {
                /* render Else Block */
                auto val = ctx[name.peek()];
                if (val.empty()) {
                    /* rendered only when */
                    renderFragment(out, mFragments[++index]);
                }
                else {
                    /* skip fragment */
                    index++;
                }
                break;
            }

            case OpenBlock: {
                /* render an open block, easy, recursive */
                auto val = ctx[name.peek()];
                auto id = index;
                if (val.empty()) {
                    /* empty value, ignore block */
                    for (; id < mActions.size(); id++)
                        if (tagString(mActions[id]) == name && mActions[id].tag == CloseBlock)
                            break;
                }
                else if (val.isArray()) {
                    /* render block multiple times */
                    for (const auto [_, e] : val) {
                        /* recursively render, retaining the index */
                        id = index + 1;
                        renderInternal(out, e, id, name);
                    }
                }
                else {
                    /* render block once */
                    renderInternal(out, val, id, name);
                }
                index = id;
                break;
            }

            case CloseBlock:
                /* closing block, do nothing */
                break;

            default:
                /* UNREACHABLE*/
                throw MustacheParseError("unknown tag type: ", act.tag);
        }
    }

    void Mustache::renderInternal(
            Buffer &out,
            const json::Object& ctx,
            int& index,
            const String& stop) const
    {
        for (;index < mFragments.size(); index++) {
            /* enumerate all fragments and render */
            renderFragment(out, mFragments[index]);
            if (index < mActions.size()) {
                /* a fragment is always followed by a tag */
                auto act = mActions[index];
                if (!stop.empty() && tagString(act) == stop) {
                    break;
                }
                renderTag(out, act, ctx, index);
            }
        }
    }

    void Mustache::render(Buffer& ob, const json::Object& ctx) const
    {
        int index{0};
        String stop{nullptr};
        renderInternal(ob, ctx, index, stop);
    }

    String Mustache::render(const json::Object& ctx) {
        Buffer ob{mBody.size()+(mBody.size()>>1)};
        Ego.render(ob, ctx);
        return String{ob};
    }

    Mustache Mustache::fromString(suil::String &&str)
    {
        Mustache m(std::move(str));
        m.parse();
        return std::move(m);
    }

    Mustache Mustache::fromFile(const char *path)
    {
        return Mustache::fromString(fs::readall(path));
    }

    MustacheCache MustacheCache::sCache{};

    MustacheCache::CacheEntry::CacheEntry(CacheEntry &&o) noexcept
        : name(std::move(o.name)),
          path(std::move(o.path)),
          tmpl(std::move(o.tmpl))
    {}

    MustacheCache::CacheEntry& MustacheCache::CacheEntry::operator=(CacheEntry &&o) noexcept
    {
        if (this != &o) {
            name = std::move(o.name);
            path = std::move(o.path);
            tmpl = std::move(o.tmpl);
        }
        return Ego;
    }

    const Mustache& MustacheCache::CacheEntry::reload()
    {
        if (!fs::exists(path())) {
            // template required to exist
            throw MustacheParseError("attempt to reload template '",
                                    (std::string_view) path(), "' which does not exist");
        }

        struct stat st{};
        stat(path.data(), &st);
        auto mod    = (time_t) st.st_mtim.tv_sec;
        if (mod != lastMod) {
            // template has been modified, reload
            lastMod = mod;
            tmpl = Mustache::fromFile(path());
        }
        return tmpl;
    }

    const Mustache& MustacheCache::load(const suil::String &&name)
    {
        auto it = Ego.mCached.find(name);
        if (it != Ego.mCached.end()) {
            // found on cache, reload cache if possible
            return it->second.reload();
        }
        // create new cache entry
        CacheEntry entry{name.dup(), catstr(Ego.options.root, "/", name)};
        entry.reload();
        auto tmp = Ego.mCached.emplace(entry.name.peek(), std::move(entry));
        return tmp.first->second.reload();
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

namespace sj = suil::json;

TEST_CASE("Mustache tests", "[template][mustache]")
{
    using Expected = std::pair<suil::Mustache::ActionType, const char*>;
    auto CheckTags = [](const suil::Mustache& m, const std::vector<Expected> expected) {
        /* same size */
        REQUIRE((m.mActions.size() == expected.size()));
        int i{0};
        for (auto& act: m.mActions) {
            // verify each action
            const auto& exp = expected[i++];
            REQUIRE(act.tag == exp.first);
            REQUIRE((m.tagString(act) == suil::String{exp.second}));
        }
    };

    auto CheckFrags = [](const suil::Mustache& m, std::vector<const char*> frags) {
        /* same size */
        REQUIRE((m.mFragments.size() == frags.size()));
        int i{0};
        for (auto& frag: m.mFragments) {
            // verify each action
            const auto& exp = frags[i++];
            REQUIRE((m.fragString(frag) == suil::String{exp}));
        }
    };

    SECTION("Parsing mustache templates") {
        /* tests parsing a valid mustache template */
        auto CheckParse = [&CheckFrags, &CheckTags](
                const char* input, std::vector<const char*> frags, const std::vector<Expected> tags) {
            suil::Mustache m;
            REQUIRE_NOTHROW((m = suil::Mustache::fromString(suil::String{input})));
            CheckTags(m, tags);
            CheckFrags(m, frags);
        };

        WHEN("Parsing valid templates") {
            // check normal tag
            CheckParse("Hello World",                 /* not tag */
                         {"Hello World"}, {});
            CheckParse("{{name}}",                    /* tag without fragments */
                         {nullptr}, {{suil::Mustache::Tag, "name"}});
            CheckParse("{{name}}{{lastname}}",        /* tags without fragments */
                         {nullptr, nullptr}, {{suil::Mustache::Tag, "name"},
                                              {suil::Mustache::Tag, "lastname"}});
            CheckParse("{{    world   }}",
                         {nullptr}, {{suil::Mustache::Tag, "world"}});
            CheckParse("Hello {{world}}",
                         {"Hello "}, {{suil::Mustache::Tag, "world"}});
            CheckParse("Hello {{world}}, its awesome",
                         {"Hello ", ", its awesome"}, {{suil::Mustache::Tag, "world"}});
            CheckParse("Hello {{name}}, welcome to {{world}} of {{planet}}",
                         {"Hello ", ", welcome to ", " of "},
                         {{suil::Mustache::Tag, "name"},
                          {suil::Mustache::Tag, "world"},
                          {suil::Mustache::Tag, "planet"}});

            // check un-escape tag
            CheckParse("{{{name}}}",                   /* tag without fragments */
                         {nullptr}, {{suil::Mustache::UnEscapeTag, "name"}});
            CheckParse("{{&name}}",                    /* tag without fragments */
                         {nullptr}, {{suil::Mustache::UnEscapeTag, "name"}});

            // check OpenBlock tag
            CheckParse("{{#name}}{{/name}}",           /* empty block */
                         {nullptr, nullptr},
                         {{suil::Mustache::OpenBlock,  "name"},
                          {suil::Mustache::CloseBlock, "name"}});
            CheckParse("{{#name}}Carter{{/name}}",     /* fragment between block */
                         {nullptr, "Carter"},
                         {{suil::Mustache::OpenBlock,  "name"},
                          {suil::Mustache::CloseBlock, "name"}});
            CheckParse("{{#name}} frag1 {{tag1}} frag2 {{tag2}} frag3 {{/name}}",     /* fragment between block */
                         {nullptr, " frag1 ", " frag2 ", " frag3 "},
                         {{suil::Mustache::OpenBlock,  "name"},
                          {suil::Mustache::Tag,        "tag1"},
                          {suil::Mustache::Tag,        "tag2"},
                          {suil::Mustache::CloseBlock, "name"}});

            // check ElseBlock tag
            CheckParse("{{^name}}{{/name}}",           /* empty block */
                         {nullptr, nullptr},
                         {{suil::Mustache::ElseBlock,  "name"},
                          {suil::Mustache::CloseBlock, "name"}});
            // check ElseBlock tag
            CheckParse("{{ ^name}}{{\t/name\t}}",           /* empty block */
                         {nullptr, nullptr},
                         {{suil::Mustache::ElseBlock,  "name"},
                          {suil::Mustache::CloseBlock, "name"}});

            CheckParse("{{^name}}Carter{{/name}}",     /* fragment between block */
                         {nullptr, "Carter"},
                         {{suil::Mustache::ElseBlock,  "name"},
                          {suil::Mustache::CloseBlock, "name"}});
            CheckParse("{{#name}} frag1 {{tag1}} frag2 {{tag2}} frag3 {{/name}}",     /* fragment between block */
                         {nullptr, " frag1 ", " frag2 ", " frag3 "},
                         {{suil::Mustache::OpenBlock,  "name"},
                          {suil::Mustache::Tag,        "tag1"},
                          {suil::Mustache::Tag,        "tag2"},
                          {suil::Mustache::CloseBlock, "name"}});

            // check Ignore tag
            CheckParse("{{!name}}{{!name}}",           /* Ignored block */
                         {nullptr, nullptr},
                         {{suil::Mustache::Ignore, "name"},
                          {suil::Mustache::Ignore, "name"}});
            CheckParse("{{!//comment?\n/* yes*/}}{{!name}}",           /* can have any characters, except delimiter */
                         {nullptr, nullptr},
                         {{suil::Mustache::Ignore, "//comment?\n/* yes*/"},
                          {suil::Mustache::Ignore, "name"}});


            // check Partial tag
            CheckParse("{{> name}}{{\t>\tname}}",           /* Partial */
                         {nullptr, nullptr},
                         {{suil::Mustache::Partial,  "name"},
                          {suil::Mustache::Partial, "name"}});
            // check Partial tag
            CheckParse("{{> index.html}}{{>includes/_header.html}}",           /* file names */
                         {nullptr, nullptr},
                         {{suil::Mustache::Partial,  "index.html"},
                          {suil::Mustache::Partial, "includes/_header.html"}});
        }

        WHEN("Parsing invalid templates") {
            // test parsing of invalid templates
            REQUIRE_THROWS(suil::Mustache::fromString("{{tag}"));   /* invalid closing tag */
            REQUIRE_THROWS(suil::Mustache::fromString("{{2tag}}")); /* tags must start with letters or '_' */
            REQUIRE_THROWS(suil::Mustache::fromString("{{_tagPrefix Postfix}}")); /* there can be no spaces between tag names */
            REQUIRE_THROWS(suil::Mustache::fromString("{{tag%&}}")); /* characters, numbers and underscore only supported in tag name */
            REQUIRE_THROWS(suil::Mustache::fromString("{{/tag}}"));  /* floating block close tag not supported  */
            REQUIRE_THROWS(suil::Mustache::fromString("{{#tag}}"));  /* unclosed block tag to supported */
            REQUIRE_THROWS(suil::Mustache::fromString("{{^tag}}"));  /* unclosed block tag to supported */
            /* inner blocks must be closed before outter blocks */
            REQUIRE_THROWS(suil::Mustache::fromString("{{#tag}}{{^tag2}}{{/tag}}{{/tag2}}"));
            /* all blocks must be closed */
            REQUIRE_THROWS(suil::Mustache::fromString("{{#tag}}{{^tag2}}{{/tag2}}{{tag}}"));
            /* currently chnage delimiter block is not supported */
            REQUIRE_THROWS(suil::Mustache::fromString("{{=<% %>=}}"));
        }
    }

    SECTION("Rendering templates") {
        /* Test rendering mutache templates */
        WHEN("Rendering with valid context") {
            /* rendering normal tags */
            suil::Mustache m = suil::Mustache::fromString("Hello {{name}}");
            auto rr = m.render(sj::Object(sj::Obj, "name", "Carter"));
            REQUIRE(rr == suil::String{"Hello Carter"});
            m = suil::Mustache::fromString("Hello {{name}}, welcome to {{planet}}!");
            rr = m.render(sj::Object(sj::Obj,
                                       "name", "Carter",
                                       "planet", "Mars"));
            REQUIRE(rr == suil::String{"Hello Carter, welcome to Mars!"});
            /* different variable types */
            m = suil::Mustache::fromString("Test variable types {{type}}={{value}}");
            rr = m.render(sj::Object(sj::Obj,
                                       "type", "int", "value", 6));
            REQUIRE(rr == suil::String{"Test variable types int=6"});
            rr = m.render(sj::Object(sj::Obj,
                                       "type", "float", "value", 5.66));
            REQUIRE(rr == suil::String{"Test variable types float=5.660000"});
            rr = m.render(sj::Object(sj::Obj,
                                       "type", "bool", "value", true));
            REQUIRE(rr == suil::String{"Test variable types bool=true"});
            rr = m.render(sj::Object(sj::Obj,
                                       "type", "string", "value", "World"));
            REQUIRE(rr == suil::String{"Test variable types string=World"});
            /* template referencing same key */
            m = suil::Mustache::fromString("{{name}} is {{name}}, {{name}}'s age is {{age}}, {{age}} hah!");
            rr = m.render(sj::Object(sj::Obj,
                                       "name", "Carter", "age", 45));
            REQUIRE(rr == suil::String{"Carter is Carter, Carter's age is 45, 45 hah!"});

            /* Parsing else block */
            m = suil::Mustache::fromString("Say {{^cond}}Hello{{/cond}}");
            rr = m.render(sj::Object(sj::Obj, "cond", false));
            REQUIRE(rr == suil::String{"Say Hello"});
            rr = m.render(sj::Object(sj::Obj, "cond", true));
            REQUIRE(rr == suil::String{"Say "});
            m = suil::Mustache::fromString("Your name is {{^cond}}Carter{{/cond}}");
            rr = m.render(sj::Object(sj::Obj,"cond", true));
            REQUIRE(rr == suil::String{"Your name is "});
            rr = m.render(sj::Object(sj::Obj));
            REQUIRE(rr == suil::String{"Your name is Carter"});
            rr = m.render(sj::Object(sj::Obj, "cond", nullptr));
            REQUIRE(rr == suil::String{"Your name is Carter"});

            /* blocks */
            m = suil::Mustache::fromString("{{#data}}empty{{/data}}");
            rr = m.render(sj::Object(sj::Obj, "data", true));
            REQUIRE(rr == "empty");
            rr = m.render(sj::Object(sj::Obj, "data", false));
            REQUIRE(rr == "");
            rr = m.render(sj::Object(sj::Obj, "data", 5));
            REQUIRE(rr == "empty");
            rr = m.render(sj::Object(sj::Obj, "data", sj::Object(sj::Arr)));
            REQUIRE(rr == "");
            rr = m.render(sj::Object(sj::Obj,
                                       "data", sj::Object(sj::Arr, 0)));
            REQUIRE(rr == "empty");
            rr = m.render(sj::Object(sj::Obj,
                                       "data", sj::Object(sj::Arr, 0, true)));
            REQUIRE(rr == "emptyempty");
            rr = m.render(sj::Object(sj::Obj,
                                       "data", sj::Object(sj::Arr, 0, "true", true, false)));
            REQUIRE(rr == "emptyemptyemptyempty");

            /* more blocks */
            m = suil::Mustache::fromString("{{#numbers}}The number is {{number}}!\n{{/numbers}}");
            rr = m.render(sj::Object(sj::Obj, "numbers", sj::Object(sj::Arr,
                                                                          sj::Object(sj::Obj, "number", 1)
            )));
            REQUIRE(rr == "The number is 1!\n");
            rr = m.render(sj::Object(sj::Obj, "numbers", sj::Object(sj::Arr,
                                                                          sj::Object(sj::Obj, "number", 1),
                                                                          sj::Object(sj::Obj, "number", 2),
                                                                          sj::Object(sj::Obj, "number", 3),
                                                                          sj::Object(sj::Obj, "number", 4))));
            REQUIRE(rr == "The number is 1!\nThe number is 2!\nThe number is 3!\nThe number is 4!\n");
            m = suil::Mustache::fromString("{{#users}}Name: {{name}}, Email: {{email}}\n{{/users}}");
            rr = m.render(sj::Object(sj::Obj, "users", sj::Object(sj::Arr,
                                                                        sj::Object(sj::Obj, "name",   "Holly", "email", "holly@gmail.com"),
                                                                        sj::Object(sj::Obj, "name",   "Molly", "email", "molly@gmail.com"))));
            REQUIRE(rr == "Name: Holly, Email: holly@gmail.com\nName: Molly, Email: molly@gmail.com\n");
        }

        WHEN("Rendering with invalid contenxt") {
            suil::Mustache m = suil::Mustache::fromString("Hello {{name}}");
            REQUIRE_THROWS(m.render(sj::Object(sj::Obj, "lastname", "Carter")));
        }
    }
}

#endif
