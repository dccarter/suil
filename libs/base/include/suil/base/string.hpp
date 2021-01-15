//
// Created by Mpho Mbotho on 2020-10-07.
//

#ifndef SUIL_BASE_STRING_HPP
#define SUIL_BASE_STRING_HPP

#include "suil/base/data.hpp"


#include <cstdint>
#include <cstring>
#include <map>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace suil {

    class Buffer;
    class Wire;

    /**
     * A light-weight string abstraction
     */
    class String {
    public:
        static constexpr size_t npos{(size_t) -1};

        /**
         * creates an empty string
         */
        String();

        /**
         * creates a string of with character \param c duplicated
         * \param n times
         * @param c the character to initialize the string with
         * @param n the initial size of the string
         */
        explicit String(char c, size_t n);

        /**
         * creates a view of the given c-style string. does not own
         * the memory and must not outlive the memory
         * @param str the string to reference
         */
        String(const char *str) noexcept;

        /**
         * creates a string from the given string view
         * @param str the string view to reference or whose buffer must be own
         * @param own if false, a reference to the string view buffer will be kept
         * otherwise the reference will be own and be free when the String is being destroyed
         */
        explicit String(const strview str);

        /**
         * creates a string from the given standard string
         * @param str the standard string to reference or duplicate
         * @param own if true, the standard string will be duplicated,
         * otherwise a reference to the string will be used
         */
        explicit String(const std::string& str, bool own = false);

        /**
         * creates a new string referencing the given string and constrained
         * to the given size.
         * @param str the buffer to reference
         * @param len the size of the buffer visible to the string
         * @param own true if the string should assume ownership of the buffer
         */
        explicit String(const char *str, size_t len, bool own = true);

        /**
         * creates a string by referencing the buffer of an output buffer
         * @param b the output buffer to be reference
         * @param own if true the output buffer will be released to this String
         */
        explicit String(Buffer& b, bool own = true);

        /**
         * creates a string by referencing the buffer of an output buffer
         * @param b the output buffer to be reference
         * @param own if true the output buffer will be released to this String
         */
        explicit String(const Buffer& b);

        /**
         * moves construct string \param s to this string
         * @param s
         */
        String(String&& s) noexcept;

        /**
         * move assign string \param s to this string
         * @param s
         * @return
         */
        String& operator=(String&& s) noexcept;

        /**
         * copy construct string \param s into this string
         * @param s string to copy
         */
        String(const String& s);

        /**
         * copy assign string \param s into this string
         * @param \s the string to copy
         */
        String& operator=(const String& s);

        /**
         * duplicate current string, returning a new string
         * with it's own buffer
         */
        String dup() const;

        /**
         * take a reference to this string
         * @return a string which reference's this string's buffer
         * but does not own it
         */
        String peek() const;

        /**
         * transform all characters in the string to uppercase
         */
        void toupper();

        /**
         * transform all characters in this string to lowercase
         */
        void tolower();

        /**
         * check if the string references any buffer with a size greater than 0
         * @return true if string refers to a valid of size greater than 0
         */
        bool empty() const;

        /**
         * boolean cast operator
         * @return \see String::empty
         */
        inline operator bool() const {
            return !empty();
        }

        /**
         * strview cast operator
         * @return string view whose data is the buffer referred to by this string
         */
        operator strview() const {
            return strview{m_str, m_len};
        }

        /**
         * eq equality operator - compares two string for equality
         * @param s string to compare against
         * @return true if the two strings are equal, false otherwise
         */
        bool operator==(const String& s) const;

        /**
         * neq equality operator - compares two string for equality
         * @param s string to compare against
         * @return true if the two strings are not equal, false otherwise
         */
        bool operator!=(const String& s) const {
            return !(Ego == s);
        }

        /**
         * find the occurrence of the given character, \param ch in the string,
         * searching from the beginning of the string towards the end
         * @param ch the character to search
         * @return the position of the first occurrance of the character if
         * found, otherwise, String::npos is returned
         */
        size_t find(char ch) const;

        size_t find(const String& str) const;

        /**
         * find the occurrence of the given character, \param ch in the string,
         * searching from the end of the string towards the beginning
         * @param ch the character to search
         * @return the position of the first occurrence of the character if
         * found, otherwise, String::npos is returned
         */
        size_t rfind(char ch) const;

        size_t rfind(const String& str) const;

        /**
         * get the substring of the current string
         *
         * @param from the position to start capturing the substring
         * @param nchars the number of characters to capture. If not specified
         * the capture will terminate at the last character
         * @param zc used to control how the substring is returned. if true,
         * only a string referencing the calling strings substring portion will
         * be returned, otherwise a copy of that portion will be returned
         *
         * @return the requested substring if not out of bounds, otherwise an
         * empty string is returned
         */
        String substr(size_t from, size_t nchars = 0, bool zc = true) const;

        /**
         * take a chunk of the string starting from first occurrence of the given
         * character depending on the find direction
         * @param ch the character to search and start chunking from
         * @param reverse if true character is search from end of string,
         * otherwise from begining of string
         */
        String chunk(const char ch, bool reverse = false) const {
            ssize_t from = reverse? rfind(ch) : find(ch);
            if (from < 0)
                return String{};
            else
                return String{&Ego.m_cstr[from], size_t(Ego.m_len-from), false};
        }

        /**
         * splits the given string at every occurrence of the given
         * delimiter
         *
         * @param the delimiter to use to split the string
         *
         * @return a vector containing pointers to the start of the
         * parts of the split string
         *
         * @note the string will not be modified, copies will be created
         */
         std::vector<String> split(const char *delim = " ");

        /**
         * splits the given string at every occurrence of the given
         * delimiter
         *
         * @param the delimiter to use to split the string
         *
         * @return a vector containing pointers to the start of the
         * parts of the split string
         *
         * @note the string will be modified, null will be added to terminate the
         * tokens
         */
         ConstVec<String> tokenize(const char *delim = " ");

        /**
         * splits the given string at every occurrence of the given
         * delimiter
         *
         * @param the delimiter to use to split the string
         *
         * @return a vector containing pointers to the start of the
         * parts of the split string
         *
         * @note the string will not be modified
         */
        ConstVec<String> parts(const char *delim = " ") const;

        /**
         * remove the occurrence of the given \param strip character from the string.
         * @param strip the character to remove from the string
         * @param ends if true, only the character at the ends will be removed
         * @return a new string with the give \param character removed from the
         * string
         */
        String strip(char strip = ' ', bool ends = false) const;

        /**
         * \see String::strip
         * @param what (default: ' ') the character to strip
         * @return a new string with the give \param character removed from the
         * string
         */
        inline String trim(char what = ' ') const {
            return strip(what, true);
        }


        /**
         * compare against given c-style string
         * @param s c-style string to compare against
         * @return \see strncmp
         */
        inline int compare(const char* s, bool igc = false) const {
            size_t  len = strlen(s);
            if (len == m_len)
                return igc? strncasecmp(m_str, s, m_len) : strncmp(m_str, s, m_len);
            return m_len < len? -1 : 1;
        }

        /**
         * compare against another string
         * @param s other string to compare against
         * @return \see strncmp
         */
        inline int compare(const String& s, bool igc = false) const {
            if (s.m_len == m_len)
                return igc? strncasecmp(m_str, s.m_str, m_len) : strncmp(m_str, s.m_str, m_len);
            return m_len < s.m_len? -1 : 1;
        }

        /**
         * Determines if the given string starts with the given String \param s
         * @param s the string to check
         * @param igc (default : false) true if checking should ignore case
         * @return true this string starts with the given string, false otherwise
         */
        inline bool startsWith(const String& s, bool igc = false) const {
            if (Ego.m_len < s.m_len) return false;
            return igc? strncasecmp(Ego.m_cstr, s.m_cstr, s.m_len) == 0 :
                        strncmp(Ego.m_cstr, s.m_cstr, s.m_len) == 0;
        }

        /**
         * gt equality operator - check if this string is greater than
         * string
         * @param s
         * @return
         */
        inline bool operator>(const String& s) const {
            return Ego.compare(s) > 0;
        }

        /**
         * gte equality operator - check if this string is greater equal to
         * given string
         * @param s
         * @return
         */
        inline bool operator>=(const String& s) const {
            return Ego.compare(s) >= 0;
        }

        /**
         * le equality operator - check if this string is less than
         * string
         * @param s
         * @return
         */
        inline bool operator<(const String& s) const {
            return Ego.compare(s) < 0;
        }

        /**
         * lte equality operator - check if this string is less than or equal to
         * given string
         * @param s
         * @return
         */
        inline bool operator<=(const String& s) const {
            return Ego.compare(s) <= 0;
        }

        /**
         * \see String::cm_str
         * @return
         */
        inline const char* operator()() const {
            return Ego.c_str();
        }

        bool endswith(const char* part, bool igc = false) const;

        /**
         * get constant pointer to the reference buffer
         * @return
         */
        inline const char* data() const {
            return Ego.m_cstr;
        }

        inline Data release() {
            Data tmp{Ego.data(), Ego.size(), Ego.m_own};
            m_own = false;
            return std::move(tmp);
        }

        /**
         * get a modifiable pointer to the reference buffer
         * @return
         */
        inline char* data() {
            return Ego.m_str;
        }

        /**
         * get the string size
         * @return
         */
        inline size_t size() const {
            return Ego.m_len;
        }

        size_t hash() const;

        /**
         * add operator concatenate given parameter to this string, implementation
         * differs depending on the type \tparam T
         * @tparam T the type of the parameter being concatenated
         * @param t the value to concatenate with this string
         * @return
         */
        template <typename T>
        inline String& operator+=(T t);

        template <typename T>
        inline String operator+(const T t);

        /**
         * get the underlying buffer as a c string, if the buffer is null
         * return whatever is passed as \param nil
         * @param nil the value to return incase the buffer is null
         * @return
         */
        const char* c_str(const char *nil = "") const;

        /**
         * casts the string to the give type
         * @tparam T the type to cast to
         * @return data of type @tparam T which is equivalent to current string
         */
        template <typename T>
        explicit inline operator T() const;

        /**
         * Access the character at the given index on the string
         * @param index  the index of the character to access
         *
         * @return a character at the given index if within bounds
         *
         * @throws suil::IndexOutOfBounds thrown when the given @param index
         * is out of bounds
         */
        const char& operator[](size_t index) const;

        const char& back() const;

        const char& front() const;

        /**
         * Destroys the string. If the string owns the buffer then it
         * will be reallocated
         */
        ~String();

        struct iterator {
            iterator(const String &s, size_t pos = 0)
                    : str(s),
                      pos(pos)
            {}
            iterator operator++() { pos++; return Ego; }
            bool operator!=(const iterator &other) {
                return Ego.pos != other.pos;
            }

            char operator*() const {
                return (char) (pos < str.size() ? str[pos] : '\0');
            }

        private:
            const String& str;
            size_t        pos{0};
        };

        using const_iterator = const iterator;

        /**
         * @return iterator to the beginning of the string buffer
         */
        iterator begin() { return iterator(Ego); }

        /**
         * @return iterator to the end of the string buffer
         */
        iterator end() { return iterator(Ego, m_len); }

        /**
         * @return constant iterator to the beginning of the string buffer
         */
        const_iterator begin() const { return const_iterator(Ego); }

        /**
         * @return constant iterator to the end of the string buffer
         */
        const_iterator end() const { return const_iterator(Ego, m_len); }

        /**
         * Cast the given String to an std::string type, this will involve a copy
         * @return an std::string which is a copy of the current string
         */
        inline operator std::string() const {
            return std::string{c_str(), size()};
        }

        /**
         * Checks if the underlying string is a hex string
         * @param checkCase true
         * @return
         */
        bool ishex(int checkCase = -1 /* -1 lower, 0 no insensitive, 1 upper */);

        size_t maxByteSize() const;

        /**
         * Clears the given string. This will deallocate the underlying buffer
         */
        void clear();

    private suil_ut:
        /**
         * serialization into a wire operator
         * @param w
         * @param s
         * @return
         */
        friend
        Wire& operator<<(Wire& w, const String& s);

        /**
         * deserialization from a wire operator
         * @param w the wire to deserialize from
         * @param s a reference to the string to deserialize into
         * @return
         */
        friend
        Wire& operator>>(Wire& w, String& s);

        friend struct hasher;
        union {
            char  *m_str;
            const char *m_cstr;
        };
        friend struct CaseInsensitiveHash;
        template <DataBuf T>
        friend struct Hasher;

        uint32_t m_len{0};
        bool     m_own{false};
        mutable size_t   m_hash{0};
    };

    /**
     * std string keyed map case sensitive comparator
     */
    struct std_map_eq {
        inline bool operator()(const std::string& l, const std::string& r) const
        {
            return std::equal(l.begin(), l.end(), r.begin(), r.end());
        }
    };

    /**
     * suil string (\class String) keyed map case sensitive equality comparer
     */
    struct CaseSensitiveCompare {
        inline bool operator()(const String& l, const String& r) const
        {
            return l == r;
        }
    };

    /**
     * suil string (\class String) keyed map case insensitive comparator
     */
    struct Compare {
        inline bool operator()(const String& l, const String& r) const
        {
            return l < r;
        }
    };

    /**
     * suil string (\class String) keyed map case in-sensitive equality comparer
     */
    struct CaseInsensitiveCompare {
        inline bool operator()(const String& l, const String& r) const
        {
            if (l.data() != nullptr) {
                return ((l.data() == r.data()) && (l.size() == r.size())) ||
                       (strncasecmp(l.data(), r.data(), std::min(l.size(), r.size())) == 0);
            }
            return (l.data() == r.data()) && (l.size() == r.size());
        }
    };

    struct CaseInsensitiveHash {
        size_t operator()(const String& key) const {
            constexpr auto init = std::size_t((sizeof(std::size_t) == 8) ? 0xcbf29ce484222325 : 0x811c9dc5);
            constexpr auto multiplier = std::size_t((sizeof(std::size_t) == 8) ? 0x100000001b3 : 0x1000193);
            if (key.m_hash != 0) {
                return key.m_hash;
            }

            std::size_t hash = init;
            for (char i : key) {
                hash ^= ::toupper(i);
                hash *= multiplier;
            }
            key.m_hash = hash;
            return hash;
        }
    };

    template <>
    struct Hasher<String> {
        inline size_t operator()(const String& key) const {
            if (key.m_hash) {
                return key.m_hash;
            }

            auto hash = std::hash<strview>{}(strview{key.data(), key.size()});
            key.m_hash = hash;
            return hash;
        }
    };

    struct CaseSensitive {
        using comparer_t = CaseSensitiveCompare;
        using hasher_t   = Hasher<String>;
    };

    struct CaseInsensitive {
        using comparer_t = CaseInsensitiveCompare;
        using hasher_t   = CaseInsensitiveHash;
    };

    /**
     * Definition of an unordered \class String keyed map
     *
     * @tparam T the type of data held in the map
     * @tparam Cs the equality comparison of the map keys
     */
    template <typename T, typename Cs = CaseSensitive>
    using UnorderedMap = std::unordered_map<String, T, typename Cs::hasher_t, typename Cs::comparer_t>;

    /**
     * Definition of an ordered \class String keyed map
     *
     * @tparam T the type of data held in the map
     * @tparam Cs the equality comparison of the map keys
     */
    template <typename T, typename Cs = CaseSensitive>
    using Map = std::map<String, T, typename Cs::hasher_t, typename Cs::comparer_t>;

    /**
     * @brief Converts a string to a decimal number. Floating point
     * numbers are not supported
     * @param str the string to convert to a number
     * @param base the numbering base to the string is in
     * @param min expected minimum value
     * @param max expected maximum value
     * @return a number converted from given string
     */
    int64_t strtonum(const String &str, int base, long long int min, long long int max);

    /**
     * compares \param orig string against the other string \param o
     *
     * @param orig the original string to match
     * @param o the string to compare to
     *
     * @return true if the given strings are equal
     */
    inline bool strmatchany(const char *orig, const char *o) {
        return strcmp(orig, o) == 0;
    }

    /**
     * match any of the given strings to the given string \param l
     * @tparam A always const char*
     * @param l the string to match others against
     * @param r the first string to match against
     * @param args comma separated list of other string to match
     * @return true if any ot the given string is equal to \param l
     */
    template<typename... A>
    inline bool strmatchany(const char *l, const char *r, A... args) {
        return strmatchany(l, r) || strmatchany(l, std::forward<A>(args)...);
    }

    /**
     * compare the first value \p l against all the other values (smilar to ||)
     * @tparam T the type of the values
     * @tparam A type of other values to match against
     * @param l the value to match against
     * @param args comma separated list of values to match
     * @return true if any
     */
    template<typename T, typename... A>
    inline bool matchany(const T l, A... args) {
        static_assert(sizeof...(args), "at least 1 argument is required");
        return ((args == l) || ...);
    }

    /**
     * converts the given string to a number
     * @tparam T the type of number to convert to
     * @param str the string to convert to a number
     * @return the converted number
     *
     * @throws \class Exception when the string is not a valid number
     */
    template<std::integral T>
    auto to_number(const String& str) -> T {
        return T(suil::strtonum(str, 10, INT64_MIN, INT64_MAX));
    }

    /**
     * converts the given string to a number
     * @tparam T the type of number to convert to
     * @param str the string to convert to a number
     * @return the converted number
     *
     * @throws runtime_error when the string is not a valid number
     */
    template<std::floating_point T>
    auto to_number(const String& str) -> T {
        double f;
        char *end;
        f = strtod(str.data(), &end);
        if (errno || *end != '\0')  {
            throw std::runtime_error(errno_s);
        }
        return (T) f;
    }

    /**
     * convert given number to string
     * @tparam T the type of number to convert
     * @param v the number to convert
     * @return converts the number to string using std::to_string
     */
    template<arithmetic T>
    inline auto tostr(T v) -> String {
        auto tmp = std::to_string(v);
        return String(tmp.c_str(), tmp.size(), false).dup();
    }

    /**
     * converts given string to string
     * @param str
     * @return a peek of the given string
     */
    inline String tostr(const String& str) {
        return std::move(str.peek());
    }

    /**
     * converts the given c-style string to a \class String
     * @param str the c-style string to convert
     * @return a \class String which is a duplicate of the
     * given c-style string
     */
    inline String tostr(const char *str) {
        return String{str}.dup();
    }

    /**
     * converts the given std string to a \class String
     * @param str the std string to convert
     * @return a \class String which is a duplicate of the
     * given std string
     */
    inline String tostr(const std::string& str) {
        return String(str.c_str(), str.size(), false).dup();
    }

    /**
     * cast \class String to number of given type
     * @tparam T the type of number to cast to
     * @param data the string to cast
     * @param to the reference that will hold the result
     */
    template <arithmetic T>
    inline void cast(const String& data, T& to) {
        to = suil::to_number<T>(data);
    }

    /**
     * cast the given \class String to a boolean
     * @param data the string to cast
     * @param to reference to hold the result. Will be true if the string is
     * 'true' or '1'
     */
    inline void cast(const String& data, bool& to) {
        to = (data.compare("1") == 0) ||
             (data.compare("true", true) == 0);
    }

    /**
     * casts the given \class String to a c-style string
     * @param data the string to cast
     * @param to the destination pointer, it returns a pointer
     * to the strings buffer
     */
    inline void cast(const String& data, const char*& to) {
        to = data();
    }

    /**
     * casts the given \class String into an std string
     * @param data the string to cast
     * @param to reference to hold the output string
     */
    inline void cast(const String& data, std::string& to) {
        to = std::move(std::string(data.data(), data.size()));
    }

    /**
     * \see String::peek
     * @param data
     * @param to
     */
    inline void cast(const String& data, String& to) {
        to = data.peek();
    }

    template <typename T>
    inline String::operator T() const {
        T to;
        suil::cast(*this, to);
        return to;
    };

    /**
     * Generate random bytes
     *
     * @param size the number of bytes to generate
     *
     * @return generated random bytes formatted as hex
     */
    String randbytes(size_t size);

    /**
     * Converts the given buffer to a hex string
     * @param in the buffer to convert to a hex string
     * @param ilen the length of the buffer
     * @return a hex string representing the given buffer
     */
    String hexstr(const uint8_t *in, size_t ilen);

    /**
     * Converts the given hex string to its binary representation
     *
     * @param str the hex string to load into a byte buffer
     * @param out the byte buffer to load string into
     * @param olen the size of the output buffer (MUST be at least str.size()/2)
     */
    void bytes(const String &str, uint8_t *out, size_t olen);

/**
 * A macro that can (AND should) be used when printing suil strings
 */
#define _PRIs(s) int(s.size()), s()
#define PRIs "%.*s"

}

inline std::ostream& operator<<(std::ostream& os, const suil::String& str)
{
    os << std::string_view(str);
    return os;
}

inline suil::String operator "" _str(const char* str, size_t len) {
    if (len) {
        return suil::String{str, len, false}.dup();
    }
    return nullptr;
}

#endif //SUIL_BASE_STRING_HPP
