//
// Created by Mpho Mbotho on 2020-12-20.
//

#include "suil/http/server/jwtauth.hpp"

namespace suil::http::server {

    JwtAuthorization::Context::Context()
    {
        memset(&Ego._flags, 0, sizeof(Ego._flags));
    }

    void JwtAuthorization::Context::authorize(Jwt jwt, JwtUse use)
    {
        Ego._jwt = std::move(jwt);
        Ego._jwt.iat(time(nullptr));
        if (Ego._jwt.exp() == 0) {
            auto now = time(nullptr);
            Ego._jwt.exp(now + jwtAuth->_keyAndTokenExpiry.expiry);
        }
        Ego.sendTok = std::move(use);
        Ego._flags.encode = 1;
        Ego._flags.requestAuth = 0;
        Ego._actualToken = Ego._jwt.encode(jwtAuth->_keyAndTokenExpiry.key);
    }

    bool JwtAuthorization::Context::authorize(String token)
    {
        Jwt tok;
        if (Jwt::decode(tok, token, jwtAuth->_keyAndTokenExpiry.key)) {
            // token successfully decoded, check if not expired
            auto now = time(nullptr);
            auto expiry = jwtAuth->_keyAndTokenExpiry.expiry;
            if ((expiry <= 0) or ((tok.exp() > now) and ((tok.iat() + expiry) > now))) {
                // token is valid, authorize request
                Ego._jwt = std::move(tok);
                Ego._flags.encode  = 1;
                Ego._flags.requestAuth = 0;
                Ego._actualToken = std::move(token);
                return true;
            }
        }

        return false;
    }

    Status JwtAuthorization::Context::authenticate(Status status, String msg)
    {
        Ego.sendTok.use = JwtUse::NONE;
        Ego._flags.encode = 0;
        Ego._flags.requestAuth = 1;
        if (msg) {
            Ego._tokenHdr = std::move(msg);
        }

        return status;
    }

    void JwtAuthorization::Context::revoke(String redirect)
    {
        if (redirect) {
            Ego._redirectUrl = std::move(redirect);
        }
        // do not send token and delete cookie
        Ego.sendTok.use = JwtUse::NONE;
    }

    bool JwtAuthorization::Context::isJwtExpired(int64 expiry) const
    {
        auto now = time(nullptr);
        return (expiry > 0) and
               ((_jwt.exp() < now) or
                ((_jwt.iat() + expiry) < now));
    }

    void JwtAuthorization::deleteCookie(Response& resp)
    {
        Cookie cookie(Ego._use.key.peek());
        cookie.value("");
        cookie.domain(Ego._domain.peek());
        cookie.path(Ego._path.peek());
        cookie.expires(time(NULL)-2);
        resp.setCookie(std::move(cookie));
    }

    void JwtAuthorization::authRequest(Response& resp, String msg)
    {
        deleteCookie(resp);
        resp.header("WWW-Authenticate", Ego._authenticate.peek());
        throw HttpError(http::Unauthorized);
    }

    void JwtAuthorization::before(Request& req, Response& resp, Context& ctx)
    {
        ctx.jwtAuth = this;
        if (req.attrs().Authorize) {
            String tokenHdr;
            if (Ego._use.use == JwtUse::HEADER) {
                const auto& auth = req.header(Ego._use.key);
                if (auth.empty() || !auth.startsWith("Bearer ", true)) {
                    // user is not authorized
                    Ego.authRequest(resp);
                }

                tokenHdr = auth.peek();
                ctx._actualToken = auth.substr(7);
            }
            else {
                // token is provided in cookies
                const auto& auth = req.cookie(Ego._use.key);
                if (!auth) {
                    // user not authorized
                    authRequest(resp);
                }
                tokenHdr = {};
                ctx._actualToken = auth.peek();
            }

            if (!ctx._actualToken) {
                // no need to proceed with invalid token
                authRequest(resp);
            }

            bool isValid{false};
            auto& keyExpiry = Ego.key(req.routeId());
            try {
                isValid = Jwt::decode(ctx._jwt, ctx._actualToken, keyExpiry.key);
            }
            catch (...) {
                // decoding token throws
                authRequest(resp, "Invalid authorization token");
            }

            if (!isValid or (ctx.isJwtExpired(keyExpiry.expiry))) {
                // token either invalid or has expired
                authRequest(resp);
            }

            if (!req.attrs().Authorize.check(ctx._jwt.roles())) {
                // token does not have permission to access resource
                authRequest(resp, "Access to resource denied");
            }

            ctx._tokenHdr = std::move(tokenHdr);
        }
    }

    void JwtAuthorization::after(Request&, Response& resp, Context& ctx)
    {
        ctx.jwtAuth = this;
        if (ctx.sendTok.use == JwtUse::HEADER) {
        // append token to request header
            resp.header(Ego._use.key, suil::catstr("Bearer ", ctx._actualToken));
        }
        else if (ctx.sendTok.use == JwtUse::COOKIE) {
            // create cooke for token
            Cookie ck{Ego._use.key.peek()};
            ck.domain(Ego._domain.peek());
            ck.path(Ego._path.peek());
            ck.value(std::move(ctx._actualToken));
            ck.expires(time(nullptr));
            resp.setCookie(std::move(ck));
        }
        else if (ctx._redirectUrl) {
            resp.redirect(Status::Found, std::move(ctx._redirectUrl));
            deleteCookie(resp);
        }
        else if (ctx._flags.requestAuth) {
            // send authentication request
            resp.header("WWW-Authenticate", Ego._authenticate.peek());
            if (ctx._tokenHdr) {
                if (resp.isStatus()) {
                    throw HttpError(resp.isStatus()?http::Unauthorized : resp.status(), ctx._tokenHdr);
                }
            }
            throw HttpError(resp.isStatus()?http::Unauthorized : resp.status());
        }
    }

    const JwtAuthorization::KeyWithTokenExpiry& JwtAuthorization::key(uint32 route) const
    {
        auto it = _routeKeys.find(route);
        if (it == _routeKeys.end()) {
            return _keyAndTokenExpiry;
        }
        return it->second;
    }
}