//
// Created by Mpho Mbotho on 2020-10-16.
//

#ifndef SUIL_BASE_MUSTACHE_HPP
#define SUIL_BASE_MUSTACHE_HPP

#include "suil/base/buffer.hpp"
#include "suil/base/json.hpp"
#include "suil/base/logging.hpp"
#include "suil/base/string.hpp"
#include "suil/base/sio.hpp"

namespace suil {

    define_log_tag(MUSTACHE);

    DECLARE_EXCEPTION(MustacheParseError);

    /**
     * A Mustache template parser. This class can be used to parse and
     * render mustache templates
     */
    class Mustache : LOGGER(MUSTACHE) {
    public:
        Mustache() = default;
        Mustache(const Mustache&) = delete;
        Mustache& operator=(const Mustache&) = delete;

        Mustache(Mustache&&) noexcept;
        Mustache&operator=(Mustache&&) noexcept;

        /**
         * Parse a mustache template from the given file path
         * @param path the path to a file with mustache template
         * @return an instance of \class Mustache which represents
         * the parsed template from file
         *
         * @throws MustacheParseError thrown when parsing the template fails
         */
        static Mustache fromFile(const char *path);

        /**
         * Parses a mustache template from the given string
         * @param str the string with mustache template to parse
         * @return an instance of \class Mustache which represents
         * the parsed template from the string
         *
         * @throws MustacheParseError thrown when parsing the template fails
         */
        static Mustache fromString(String&& str);

        /**
         * Renders a parsed mustache template using the given json object and returns
         * a rendered string
         * @param ctx the object with the parameters to use when rendering
         * @return a string representation of the rendered template
         *
         * @throws MustacheParseError thrown when rendering the template and
         * parsing subsections fail
         */
        String render(const json::Object& ctx);

        /**
         * Renders a parsed mustache template using the given json object into
         * the given output buffer
         * @param ob the buffer to render the template into
         * @param ctx a json object with rendering parameters
         *
         * @throws MustacheParseError thrown when rendering the template and parsing
         * subsections fails
         */
        void render(Buffer& ob, const json::Object& ctx) const;

    private suil_ut:
        sptr(Mustache);

        friend struct MustacheCache;
        Mustache(String&& str)
                : mBody(std::move(str))
        {};

        enum ActionType {
            Ignore,
            Tag,
            UnEscapeTag,
            OpenBlock,
            CloseBlock,
            ElseBlock,
            Partial
        };

        class Action {
        friend class Mustache;
        private suil_ut:
            ActionType  tag;
            size_t      first;
            size_t      last;
            size_t      pos;
            Action(ActionType t, size_t f, size_t l, size_t p = 0)
                    : tag(t), first(f), last(l), pos(p)
            {}
        };

        class Parser {
            friend class Mustache;
            char    *data{nullptr};
            size_t   size{0};
            size_t   pos{0};
            size_t   frag{0};

            Parser(String& s)
                    : data(s.data()),
                      size(s.size())
            {}

            inline bool eof() {
                return pos >= size;
            }

            inline char next() {
                return data[pos++];
            }

            inline char peek(int off = 0) {
                size_t tmp{pos+off};
                return (char) ((tmp >=size)? '\0' : data[tmp]);
            }

            inline void eatSpace() {
                while (!eof() && isspace(data[pos]))
                    pos++;
            }

            inline std::string_view substr(size_t first, size_t last) {
                if (first < size)
                    return std::string_view{&data[first], std::min(size-first, last-first)};
                return std::string_view{"invalid substr"};
            }

            inline std::string_view tagName(const Action& a) {
                return std::string_view{&data[a.first], a.last-a.first};
            }
        };
        using Fragment = std::pair<size_t,size_t>;
        using BlockPositions = std::vector<size_t>;

        void escape(Buffer& out, const String& in) const;

        void renderFragment(Buffer& out, const Fragment& frag) const;
        void renderInternal(Buffer& out, const json::Object& ctx, int& index, const String& stop) const;
        void renderTag(Buffer& out, const Action& tag, const json::Object& ctx, int& index) const;

        void parse();
        void dump();
        void readTag(Parser& p, BlockPositions& blocks);
        inline bool isFragEmpty(const Fragment& frag) const {
            return frag.second == frag.first;
        }

        inline bool isTagEmpty(const Action& act) const {
            return act.last == act.first;
        }

        inline String fragString(const Fragment& frag) const {
            return isFragEmpty(frag)?
                   nullptr : String{&mBody.data()[frag.first], frag.second-frag.first, false};
        }

        inline String tagString(const Action& act) const {
            return isTagEmpty(act)?
                   nullptr : String{&mBody.data()[act.first], act.last-act.first, false};
        }

        inline std::string_view tagView(const Action& act) const {
            return isTagEmpty(act)?
                   std::string_view{} : std::string_view{&mBody.data()[act.first], act.last-act.first};
        }

        std::vector<Action>   mActions;
        std::vector<Fragment> mFragments;
        String                mBody{};
    };

    /**
     * MustacheCache is used to cache compiled (or parsed) mustache
     * templates. Caching can help speed up using mustache templates
     * as parsing a template can take a bit of time
     */
    class MustacheCache final {
    public:
        struct Options {
            String root{"./res/templates"};
        };

        /**
         * Retrieve a mustache template from the cache. If the template
         * does not exist in the cache, the template file will be parsed
         * and stored in cache
         * @param name the name of the template to load (usually a filename)
         * @return a reference to the loaded mustache template
         *
         * @throws MustacheParseError thrown when parsing the template fails
         * or the template file does not exist
         */
        const Mustache& load(const String&& name);

        /**
         * Configure mustache template with the given options
         * @tparam Opts the type of setup parameters being provided
         * @param args a list of setup parameters @see \class Options
         */
        template<typename... Opts>
        void setup(Opts... args) {
            auto opts = iod::D(std::forward<Opts>(args)...);
            suil::applyOptions(options, opts);
            if (opts.has(var(root))) {
                // root directory has changed
                mCached.clear();
            }
        }

        /**
         * Get a static instance of the mustache cache
         * @return An instance of the cache manager
         */
        static MustacheCache& get() { return sCache; }
    private:

        MustacheCache() = default;
        MustacheCache(const MustacheCache&) = delete;
        MustacheCache&operator=(const MustacheCache&) = delete;
        MustacheCache(MustacheCache&&) = delete;
        MustacheCache&operator=(MustacheCache&&) = delete;

        struct CacheEntry {
            CacheEntry(String&& name, String&& path)
                    : name(std::move(name)),
                      path(std::move(path))
            {}

            const Mustache& reload();

            CacheEntry(CacheEntry&& o) noexcept;

            CacheEntry& operator=(CacheEntry&& o) noexcept;

            CacheEntry(const CacheEntry&) = delete;
            CacheEntry& operator=(const CacheEntry&) = delete;

            String       name;
            String       path;
            Mustache     tmpl;
            time_t       lastMod{0};
        };

        Options options{"./res/templates"};
        UnorderedMap<CacheEntry>  mCached{};
        static MustacheCache      sCache;
    };
}
#endif //SUIL_BASE_MUSTACHE_HPP
