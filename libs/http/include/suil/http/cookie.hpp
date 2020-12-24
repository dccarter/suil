//
// Created by Mpho Mbotho on 2020-12-15.
//

#ifndef SUIL_HTTP_SERVER_COOKIE_HPP
#define SUIL_HTTP_SERVER_COOKIE_HPP

#include <suil/base/string.hpp>

namespace suil::http {

    class Cookie {
    public:
        explicit Cookie(String name);
        Cookie(String name, String value);
        MOVE_CTOR(Cookie) = default;
        MOVE_ASSIGN(Cookie) = default;

        inline bool operator==(const Cookie& other) const {
            return Ego._name == other._name;
        }

        bool operator!=(const Cookie& other) const {
            return Ego._name != other._name;
        }

        inline operator bool() const {
            return !Ego.empty();
        }

        inline bool empty() const {
            return Ego._name.empty();
        }

        inline const String& name() const {
            return Ego._name;
        }

        inline const String& value() const {
            return Ego._value;
        }

        inline  Cookie& value(String v) {
            Ego._value = std::move(v);
            return Ego;
        }

        inline Cookie& value(const char* v) {
            return value(String{v}.dup());
        }

        inline const String& path() const {
            return Ego._path;
        }

        inline Cookie& path(String p) {
            Ego._path = std::move(p);
            return Ego;
        }

        inline Cookie& path(const char* p) {
            return path(String{p}.dup());
        }

        inline const String& domain() const {
            return Ego._domain;
        }

        inline Cookie& domain(String d) {
            Ego._domain = std::move(d);
            return Ego;
        }

        inline Cookie& domain(const char* d) {
            return domain(String{d}.dup());
        }

        inline bool secure() const {
            return Ego._secure;
        }

        inline Cookie& secure(bool on) {
            Ego._secure = on;
            return Ego;
        }

        inline time_t expires() const {
            return Ego._expires;
        }

        inline Cookie& expires(time_t secs) {
            if (secs > 0) {
                Ego._expires = time(nullptr) + secs;
            }
            else {
                Ego._expires = secs;
            }
            return Ego;
        }

        inline int64 maxAge() const {
            return Ego._maxAge;
        }

        inline Cookie& maxAge(int64 age) {
            Ego._maxAge = age;
            return Ego;
        }

        bool encode(Buffer& dst) const;

        ~Cookie() = default;

    private:
        DISABLE_COPY(Cookie);

        String _name{};
        String _value{};
        String _path{};
        String _domain{};
        bool   _secure{false};
        int64  _maxAge{0};
        time_t _expires{0};
    };

    using CookieJar = UnorderedMap<Cookie, CaseInsensitive>;
}
#endif //SUIL_HTTP_SERVER_COOKIE_HPP
