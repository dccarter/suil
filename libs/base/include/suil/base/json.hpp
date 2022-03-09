//
// Created by Mpho Mbotho on 2020-10-15.
//

#ifndef SUIL_BASE_JSON_HPP
#define SUIL_BASE_JSON_HPP

#include "suil/utils/utils.hpp"
#include "suil/base/string.hpp"
#include "suil/base/buffer.hpp"
#include "suil/base/blob.hpp"
#include "suil/base/logging.hpp"
#include "suil/base/meta.hpp"

#include <suil/utils/exception.hpp>

#include <iod/json.hh>

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstddef>
#include <initializer_list>

struct lua_State;

namespace suil::json {

    enum class Tag : unsigned int {
        JSON_NULL,
        JSON_BOOL,
        JSON_STRING,
        JSON_NUMBER,
        JSON_ARRAY,
        JSON_OBJECT,
    };

    struct JsonNode;

    template <typename T>
    struct CopyOut {
        CopyOut(CopyOut&&) = delete;
        CopyOut(const CopyOut&) = delete;
        CopyOut& operator=(CopyOut&&) = delete;
        CopyOut& operator=(const CopyOut&) = delete;
        CopyOut(T& ref)
                : val(ref)
        {}
        T& val;
    };

    struct Object_t {};
    static const Object_t Obj{};
    struct Array_t {};
    static const Array_t Arr{};

    struct Object {
        using ArrayEnumerator  = std::function<bool(suil::json::Object)>;
        using ObjectEnumerator = std::function<bool(const char *key, suil::json::Object)>;

        class iterator {
        public:
            iterator(JsonNode *node) : mNode(node) {}
            iterator operator++();
            bool operator!=(const iterator &other) const { return mNode != other.mNode; }
            bool operator==(const iterator &other) const { return mNode == other.mNode; }
            const std::pair<const char *, Object> operator*() const;
        private:
            JsonNode *mNode;
        };

        using const_iterator = iterator;

        Object();
        Object(std::nullptr_t)
                : Object(){}

        explicit Object(const String& str);

        explicit Object(const std::string& str);

        explicit Object(const char *str);

        explicit Object(double d);

        explicit Object(bool b);

        Object(Object_t);

        template<typename... Values>
        Object(Object_t _, Values... values)
                : Object(_) {
            Ego.set(std::forward<Values>(values)...);
        }

        template <typename ...T>
        explicit Object(const iod::sio<T...>& obj)
                : Object(json::Obj)
        {
            Ego.set(obj);
        }

        template <typename ...T>
        explicit Object(const iod::sio<T...>&& obj)
                : Object(obj)
        {}

        Object(Array_t);

        template<typename... Values>
        Object(Array_t _, Values... values)
                : Object(_) {
            Ego.push(std::forward<Values>(values)...);
        }

        template<typename T>
        explicit Object(std::vector<T> &vs)
                : Object(json::Arr) {
            for (auto v: vs)
                Ego.push(v);
        }

        template<typename T>
        explicit Object(const std::vector<T> &&vs)
                : Object(vs)
        {}

        template<arithmetic T>
        explicit Object(T t)
            : Object((double) t)
        {}

        template <typename T>
            requires (!std::is_arithmetic_v<T>)
        explicit Object(T t)
            : Object(json::Obj)
        {
            Ego.set(t);
        }

        Object(const Object &o) noexcept
            : mNode(o.mNode),
              ref(true)
        { }

        Object(Object &&o) noexcept
            : mNode(o.mNode),
              ref(o.ref)
        { o.mNode = nullptr; }

        Object &operator=(Object &&o) noexcept;

        void push(Object &&);

        Object push(const Object_t &);

        Object push(const Array_t &);

        template <typename ...T>
        void push(const iod::sio<T...>& obj) {
            Ego.push(json::Obj).set(obj);
        }

        template <typename T>
            requires std::is_base_of_v<iod::MetaType, T>
        void push(const T& obj) {
            Ego.push(json::Obj).set(obj);
        }

        template <typename T>
        void push(const std::vector<T>& v) {
            for(auto& a: v)
                Ego.push(a);
        }

        template <typename T>
        void push(const iod::Nullable<T>& a) {
            if (!a.isNull)
                Ego.push(*a);
        }

        template<typename T, typename... Values>
        void push(T t, Values... values) {
            if constexpr (std::is_same_v<json::Object, T>)
                push(std::move(t));
            else
                push(Object(t));
            if constexpr (sizeof...(values) > 0)
                push(std::forward<Values>(values)...);
        }

        template <typename ...T>
        void set(const iod::sio<T...>& obj) {
            iod::foreach(obj)|[&](auto& m) {
                if (!m.attributes().has(iod::_json_skip)) {
                    // only set non-skipped values
                    Ego.set(m.symbol().name(), m.value());
                }
            };
        }

        template <typename T>
            requires std::is_base_of_v<iod::MetaType, T>
        void set(const T& obj) {
            iod::foreach(T::Meta)|[&](auto& m) {
                if (!m.attributes().has(iod::_json_skip)) {
                    // only set non-skipped values
                    Ego.set(m.symbol().name(), m.symbol().member_access(obj));
                }
            };
        }

        template <typename T>
        void set(const char *key, const std::vector<T>& v) {
            Ego.set(key, json::Arr).push(v);
        }

        template <typename T>
        void set(const char *key, const iod::Nullable<T>& a) {
            if (!a)
                Ego.set(key, nullptr);
            else
                Ego.set(key, *a);
        }

        void set(const char *key, Object &&);

        Object set(const char *key, const Object_t &);

        Object set(const char *key, const Array_t &);

        template<typename T, typename... Values>
        void set(const char *key, T value, Values... values) {
            if constexpr (std::is_same<json::Object, T>::value)
                set(key, std::move(value));
            else
                set(key, Object(value));
            if constexpr (sizeof...(values) > 0)
                set(std::forward<Values>(values)...);
        }

        Object operator[](int index) const;

        Object operator()(const char *key, bool throwNotFound = false) const;
        Object operator()(int index, bool throwNotFound = false) const;

        Object operator[](const char* key) const;
        Object operator[](const String& key) const;
        Object get(const String& key, bool shouldThrow = true) const;

        operator bool() const;

        operator String() const {
            return String{(const char *) Ego};
        }

        operator std::string() const {
            return std::string{(const char *) Ego};
        }

        operator const char *() const;

        operator double() const;

        static Object decodeStr(const char *str, size_t &sz);
        static Object decode(const char *str, size_t &sz) {
            return decodeStr(str, sz);
        }

        template <typename T>
        static Object decode(const T& data) {
            size_t sz{data.size()};
            return Object::decodeStr(reinterpret_cast<const char *>(data.data()), sz);
        }

        template <typename T>
        static Return<Object> tryDecode(const T& data) {
            size_t sz{data.size()};
            return TryCatch<Object>(Object::decodeStr, reinterpret_cast<const char *>(data.data()), sz);
        }

        static Object fromLuaString(const String& script);

        static Object fromLuaFile(const String& file);

        void encode(iod::encode_stream &ss) const;

        template<arithmetic T>
        inline operator T() const {
            return (T) ((double) Ego);
        }

        template <typename T>
        operator std::vector<T>() const {
            if (Ego.isNull() || Ego.empty())
                return std::vector<T>{};
            if (!Ego.isArray())
                throw UnsupportedOperation("JSON object is not an array");
            std::vector<T> out;
            Ego|[&](suil::json::Object obj) {
                // cast to vector's value type
                T tmp;
                Ego.copyOut<T>(tmp, obj);
                out.push_back(std::move(tmp));
                return false;
            };

            return std::move(out);
        }

        template <typename T>
        operator iod::Nullable<T>() const {
            iod::Nullable<T> ret;
            if (!Ego.isNull() && !Ego.empty()) {
                T tmp;
                Ego.copyOut<T>(tmp, Ego);
                ret = std::move(tmp);
            }
            return std::move(ret);
        }

        template <typename ...T>
        operator iod::sio<T...>() const {
            iod::sio<T...> ret{};
            if (Ego.empty())
                return std::move(ret);
            iod::foreach(ret) | [&](auto& m) {
                // find all properties
                using _Tp = typename std::remove_reference<decltype(m.value())>::type;
                if (!m.attributes().has(iod::_json_skip)) {
                    // find attribute in current object
                    auto obj = Ego[m.symbol().name()];
                    if (obj.isNull()) {
                        if (!iod::is_nullable(m.value())  && !m.attributes().has(iod::_optional))
                            // value cannot be null
                            throw UnsupportedOperation("property '", m.symbol().name(), "' cannot be null");
                        else
                            return;
                    }

                    copyOut<_Tp>(m.value(), obj);
                }
            };

            return std::move(ret);
        }

        template<typename T, typename std::enable_if<!std::is_arithmetic<T>::value>::type* = nullptr>
        operator T() const {
            T ret{};
            if (Ego.empty())
                return std::move(ret);
            iod::foreach(T::Meta) | [&](auto& m) {
                // find all properties
                using _Tp = typename std::remove_reference<decltype(m.value())>::type;
                if (!m.attributes().has(iod::_json_skip)) {
                    // find attribute in current object
                    auto obj = Ego[m.symbol().name()];
                    if (obj.isNull()) {
                        if (!iod::is_nullable(m.value())  && !m.attributes().has(iod::_optional))
                            // value cannot be null
                            throw UnsupportedOperation("property '", m.symbol().name(), "' cannot be null");
                        else
                            return;
                    }

                    copyOut<_Tp>(m.symbol().member_access(ret), obj);
                }
            };

            return std::move(ret);
        }

        template <typename T>
        inline void copyOut(T& out, const Object& obj) const {
            if constexpr (std::is_same<T, suil::String>::value)
                out = ((String) obj).dup();
            else
                out = (T) obj;
        }

        template <typename T>
        inline T operator ||(T&& t) {
            if (Ego.empty())
                return std::forward<T>(t);
            else
                return (T) Ego;
        }

        inline String operator ||(String&& s) {
            if (Ego.empty())
                return std::move(s);
            else
                return ((String) Ego).dup();
        }

        inline std::string operator ||(std::string&& s) {
            if (Ego.empty())
                return std::move(s);
            else
                return (std::string) Ego;
        }

        bool empty() const;

        bool isBool() const;

        bool isObject() const;

        bool isNull() const;

        bool isNumber() const;

        bool isString() const;

        bool isArray() const;

        Tag type() const;

        void operator|(ArrayEnumerator f) const;

        void operator|(ObjectEnumerator f) const;

        bool operator==(const Object& other) const;

        inline bool operator!=(const Object& other) const {
            return !(Ego == other);
        }

        iterator begin();

        iterator end() { return iterator(nullptr); }

        const_iterator begin() const;

        const_iterator end() const { return const_iterator(nullptr); }

        Object weak() {
            return Object(mNode, true);
        }

        static Object fromLuaTable(lua_State* L, int index = 0);
        ~Object();

    private suil_ut:

        Object(JsonNode *node, bool ref = true)
                : mNode(node),
                  ref(ref) {};
        JsonNode *mNode{nullptr};
        bool ref{false};
    };
}

namespace suil {
    template <size_t N>
    void Blob<N>::toJson(iod::json::jstream& ss) const {
        auto s = suil::hexstr(&Ego.cbin(), Ego.size());
        iod::stringview sv(s.data(), s.size());
        json_encode_(sv, ss);
    }

    template <size_t N>
    Blob<N> Blob<N>::fromJson(iod::json::parser &p) {
        String tmp;
        p.fill(tmp);
        if (tmp.size() > N)
            throw IndexOutOfBounds("Source data cannot fit into blob");
        Blob<N> blob;
        blob.copy(tmp.data(), tmp.size());
        return std::move(blob);
    }
}

namespace iod {

    template<>
    inline json_internals::json_parser& json_internals::json_parser::fill<suil::String>(suil::String& s) {
        Ego >> '"';

        int start = pos;
        int end = pos;

        while (true) {
            while (!eof() and str[end] != '"')
                end++;

            // Count the prev backslashes.
            int sb = end - 1;
            while (sb >= 0 and str[sb] == '\\')
                sb--;

            if ((end - sb) % 2) break;
            else
                end++;
        }
        s = suil::String(str.data() + start, (size_t)(end-start), false).dup();
        pos = end+1;
        return *this;
    }

    template<>
    inline json_internals::json_parser& json_internals::json_parser::fill<suil::Data>(suil::Data& d) {
        Ego >> '"';

        int start = pos;
        int end = pos;

        while (true) {
            while (!eof() and str[end] != '"')
                end++;

            // Count the prev backslashes.
            int sb = end - 1;
            while (sb >= 0 and str[sb] == '\\')
                sb--;

            if ((end - sb) % 2) break;
            else
                end++;
        }
        suil::String tmp{};
        Ego.fill(tmp);
        auto size = (tmp.size()/2)+2;
        auto *buf = (uint8_t *) malloc(size);
        suil::bytes(tmp, buf, size);
        d = suil::Data(buf, size);
        pos = end+1;
        return *this;
    }

    template<>
    inline json_internals::json_parser& json_internals::json_parser::fill<suil::json::Object>(suil::json::Object& o) {
        // just parse into a json object
        size_t tmp = str.size();
        o = suil::json::Object::decode(&str[pos], tmp);
        pos += tmp;
        return Ego;
    }

    // Decode \o from a json string \b.
    template<typename ...T>
    inline void json_decode(sio<T...> &o, const suil::Buffer& b) {
        iod::stringview str(b.data(), b.size());
        json_decode(o, str);
    }

    template<typename O>
    inline void json_decode(std::vector<O>&o, const suil::Buffer& b) {
        iod::stringview str(b.data(), b.size());
        json_decode(o, str);
    }

    template<typename ...T>
    inline void json_decode(sio<T...> &o, const suil::String& zc_str) {
        iod::stringview str(zc_str.data(), zc_str.size());
        json_decode(o, str);
    }

    template<typename ...T>
    inline void json_decode(sio<T...> &o, const json_string& jstr) {
        iod::stringview str(jstr.str.data(), jstr.str.size());
        json_decode(o, str);
    }

    template<typename T>
        requires IsMetaType<T>
    inline void json_decode(T& o, const stringview& s) {
        json_decode<typename T::Schema, T>(o, s);
    }

    template<typename T>
        requires IsUnionType<T>
    inline void json_decode(T& o, const stringview& s) {
        json_internals::json_parser p(s);
        iod_from_json_((T *) 0, o, p);
    }

    template<typename S>
    inline void json_decode(suil::json::Object& o, const S& s) {
        auto size = static_cast<size_t>(s.size());
        o = suil::json::Object::decode(s.data(), size);
        if (size != s.size())
            throw suil::UnsupportedOperation("decoding json string failed at pos ", size);
    }

    namespace json_internals {

        template<typename S>
        inline void json_encode_(const suil::String& s, S &ss) {
            stringview sv(s.data(), s.size());
            json_encode_(sv, ss);
        }

        template<typename S>
        inline void json_encode_(const suil::Data& d, S &ss) {
            auto s = suil::hexstr(d.cdata(), d.size());
            stringview sv(s.data(), s.size());
            json_encode_(sv, ss);
        }

        template<typename S>
        inline void json_encode_(const iod::json_string& s, S &ss) {
            if (!s.str.empty()) {
                ss << s.str;
            }
            else {
                ss << "{}";
            }
        }

        template <typename S>
        inline void json_encode_(const suil::json::Object& o, S &ss) {
            o.encode(ss);
        }
    }

    inline std::string json_encode(const suil::json::Object& o) {
        encode_stream ss;
        json_internals::json_encode_(o, ss);
        return ss.move_str();
    }

    template <typename T>
        requires (iod::IsMetaType<T> or iod::IsUnionType<T>)
    inline std::string json_encode(const T& o) {
        encode_stream ss;
        json_internals::json_encode_(o, ss);
        return ss.move_str();
    }
}

namespace suil {

    namespace json {
        template<typename O>
        inline std::string encode(const Map <O> &m) {
            if (m.empty()) {
                return "{}";
            }

            iod::encode_stream ss;
            bool first{false};
            for (auto &e: m) {
                if (first) ss << ", ";
                ss << '"' << e.first << "\": ";
                iod::json_internals::json_encode_(e.second, ss);
                first = false;
            }

            return ss.move_str();
        }

        template<typename O>
        inline std::string encode(const O &o) {
            return iod::json_encode(o);
        }

        template<typename S, typename O>
        static bool trydecode(const S &s, O &o) {
            iod::stringview sv(s.data(), s.size());
            try {
                iod::json_decode(o, sv);
                return true;
            }
            catch (...) {
                sdebug("decoding json string failed: %s", Exception::fromCurrent().what());
                return false;
            }
        }

        template<typename S, typename O>
        inline void decode(const S &s, O &o) {
            iod::stringview sv(s.data(), s.size());
            iod::json_decode(o, sv);
        }

        template <typename Mt>
            requires iod::IsMetaType<Mt>
        inline void metaToJson(const Mt& o, iod::json::jstream& ss)
        {
            ss << '{';
            int i = 0;
            bool first = true;
            foreach(Mt::Meta) | [&](const auto& m) {
                if (!m.attributes().has(iod::_json_skip)) {
                    /* ignore empty entry */
                    auto& val = m.symbol().member_access(o);
                    using T = std::decay_t<decltype(val)>;
                    if (m.attributes().has(iod::_ignore) && iod::json::json_ignore<T>(val)) return;

                    if (!first) { ss << ','; }
                    first = false;
                    iod::json::json_encode_symbol(m.attributes().get(iod::_json_key, m.symbol()), ss);
                    ss << ':';
                    iod::json::json_encode_(val, ss);
                }
                i++;
            };
            ss << '}';
        }

        template <typename Mt>
            requires iod::IsUnionType<Mt>
        inline void metaToJson(const Mt& o, iod::json::jstream& ss)
        {
            iod::json_internals::json_encode_(o, ss);
        }
    }

    template <typename... T>
    Buffer& Buffer::operator<<(const iod::sio<T...>& o) {
        return (Ego << json::encode(o));
    }
}


#endif //SUIL_BASE_JSON_HPP
