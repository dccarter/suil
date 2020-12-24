//
// Created by Mpho Mbotho on 2020-12-19.
//

#ifndef LIBS_HTTP_SRC_JWT_HPP
#define LIBS_HTTP_SRC_JWT_HPP

#include <suil/http/jwt.scc.hpp>

namespace suil::http {

    DECLARE_EXCEPTION(JwtEncodeError);
    DECLARE_EXCEPTION(JwtDecodeError);

    class Jwt {
    public:
        Jwt() = default;
        explicit Jwt(String typ);

        MOVE_CTOR(Jwt) = default;
        MOVE_ASSIGN(Jwt) = default;

        template <typename T, typename... Claims>
        inline void claims(const char* key, T&& value, Claims&&... c) {
            Ego._payload.claims.set(key, std::forward<T>(value), std::forward<Claims>(c)...);
        }

        void roles(const std::vector<String>& roles);

        template <typename ...Roles>
        inline void roles(const String& role, Roles&&... roles) {
            auto rs = Ego._payload.claims["roles"];
            if (rs.empty()) {
                Ego._payload.claims.set("roles", json::Object(json::Arr,
                              role, std::forward<Roles>(roles)...));
            }
            else {
                rs.push(role, std::forward<Roles>(roles)...);
            }
        }

        inline json::Object roles() const {
            return Ego._payload.claims["roles"];
        }

        inline json::Object claims(const String& key) const {
            return Ego._payload.claims[key];
        }

        inline const String& typ() const {
            return Ego._header.typ;
        }

        inline const String& alg() const {
            return Ego._header.alg;
        }

        inline const String& iss() const {
            return Ego._payload.iss;
        }

        inline void iss(String iss){
            Ego._payload.iss = std::move(iss);
        }

        inline const String& aud() const {
            return Ego._payload.aud;
        }

        inline void aud(String aud) {
            Ego._payload.aud = std::move(aud);
        }

        inline const String sub() const {
            return Ego._payload.sub;
        }

        inline void sub(String sub) {
            Ego._payload.sub = std::move(sub);
        }

        inline const int64_t exp() const {
            return Ego._payload.exp;
        }

        inline void exp(int64_t exp) {
            Ego._payload.exp = exp;
        }

        inline const int64_t iat() const {
            return Ego._payload.iat;
        }

        inline void iat(int64_t iat) {
            Ego._payload.iat = iat;
        }

        inline const int64_t nfb() const {
            return Ego._payload.nbf;
        }

        inline void nbf(int64_t nbf) {
            Ego._payload.exp = nbf;
        }

        inline const String& jti() const {
            return Ego._payload.jti;
        }

        inline void jti(String jti) {
            Ego._payload.jti = std::move(jti);
        }

        static bool decode(Jwt& jwt, const String& jstr, const String& secret, bool fault = false);
        static bool verify(const String& zcstr, const String& secret, bool fault = false);
        String encode(const String& secret) const;

    private:
        DISABLE_COPY(Jwt);
        JwtHeader  _header{};
        JwtPayload _payload{};
    };
}
#endif //LIBS_HTTP_SRC_JWT_HPP
