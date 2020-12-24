//
// Created by Mpho Mbotho on 2020-12-17.
//

#ifndef SUIL_HTTP_SERVER_RESPONSE_HPP
#define SUIL_HTTP_SERVER_RESPONSE_HPP

#include <suil/http/cookie.hpp>
#include <suil/http/common.hpp>

#include <suil/net/chunk.hpp>
#include <suil/base/buffer.hpp>
#include <suil/base/json.hpp>
#include <suil/base/logging.hpp>


namespace suil::http {

    class WebSockApi;

namespace server {

    class Request;
    class Response;

    using ProtocolUpgrade = std::function<void(Request&, Response&)>;

    define_log_tag(HTTP_RESP);
    class Response :LOGGER(HTTP_RESP) {
    public:
        Response() = default;
        Response(Status status);
        Response(Buffer buffer);

        template <typename T>
            requires (std::is_same_v<T, json::Object> or iod::IsMetaType<T> or iod::IsUnionType<T>)
        Response(const T& t)
            : _status{http::Ok}
        {
            auto str = new std::string;
            *str = json::encode(t);
            Ego.chunk({str->data(), str->size(), 0, [str](void *) {
                delete str;
            }});
            setContentType("application/json");
        }

        template <typename T>
            requires (iod::IsMetaType<T> or iod::IsUnionType<T> or std::is_same_v<json::Object, T>)
        Response(const std::vector<T>& t)
            : _status{http::Ok}
        {
            auto str = new std::string;
            *str = json::encode(t);
            Ego.chunk({str->data(), str->size(), 0, [str](void *) {
                delete str;
            }});
            setContentType("application/json");
        }

        template <typename T>
        Response(const T& t)
            : _status{http::Ok}
        {
            Ego._body << t;
            setContentType("application/json");
        }

        MOVE_CTOR(Response) noexcept;
        MOVE_ASSIGN(Response) noexcept;

        void end(Status status = http::Ok);
        void end(Status status, Buffer buffer);
        void end(ProtocolUpgrade upgrade);

        template<typename T>
            requires (std::is_same_v<T, json::Object> or iod::IsMetaType<T> or iod::IsUnionType<T>)
        inline void end(Status status, const T& t) {
            Ego.template append(t);
            Ego._status = status;

        }

        template<typename T>
            requires (iod::IsMetaType<T> or iod::IsUnionType<T> or std::is_same_v<json::Object, T>)
        inline void end(Status status, const std::vector<T>& t) {
            Ego.template append(t);
            Ego._status = status;

        }

        template<typename T>
        inline void end(Status status, const T& t) {
            Ego._body << t;
            Ego._status = status;
            Ego._completed = true;
        }

        void redirect(Status status, String location = {});

        inline Buffer& body() {
            return Ego._body;
        }

        template <typename T>
        inline Response& operator<<(const T& t) {
            return Ego.append(t);
        }

        template <typename T>
            requires (iod::IsMetaType<T> or iod::IsUnionType<T> or std::is_same_v<json::Object, T>)
        inline Response& append(const T& t) {
            if (!Ego._body.empty()) {
                // chunk body
                auto size = Ego._body.size();
                Ego.chunk({Ego._body.release(), size, 0, [&](void *ptr){
                    // free the released pointer
                    ::free(ptr);
                }});
            }
            else {
                setContentType("application/json");
            }

            auto str = new std::string;
            *str = json::encode(t);
            Ego.chunk({str->data(), str->size(), 0, [str](void *) {
                delete str;
            }});

            return Ego;
        }

        template <typename T>
            requires (iod::IsMetaType<T> or iod::IsUnionType<T> or std::is_same_v<json::Object, T>)
        inline Response& append(const std::vector<T>& t) {
            if (!Ego._body.empty()) {
                // chunk body
                auto size = Ego._body.size();
                Ego.chunk({Ego._body.release(), size, 0, [&](void *ptr){
                    // free the released pointer
                    ::free(ptr);
                }});
            }
            else {
                setContentType("application/json");
            }

            auto str = new std::string;
            *str = json::encode(t);
            Ego.chunk({str->data(), str->size(), 0, [str](void *) {
                delete str;
            }});

            return Ego;
        }

        template <typename T>
        inline Response& append(const T& t) {
            Ego._body << t;
            return Ego;
        }

        template <typename T, typename...Args>
        inline Response& appendf(const char* fmt, const Args&... args) {
            Ego._body.appendf(fmt, args...);
            return Ego;
        }

        void header(String field, String name);

        inline void header(const char* field, const char* name) {
            Ego.header(String{field}.dup(), String{name}.dup());
        }

        const String& header(const String& name) const;

        const String& contentType() const;

        void setCookie(Cookie ck);

        size_t const size() const;

        inline bool isCompleted() const {
            return Ego._completed;
        }

        inline bool isStatus(Status status = http::Ok) {
            return Ego._status == status;
        }
        void merge(Response&& resp);
        void setContentType(String type);
        void clear();
        void clearContent();

    private suil_ut:
        DISABLE_COPY(Response);
        inline ProtocolUpgrade& operator()() { return Ego._upgrade; }
        void flushCookies();
        void chunk(net::Chunk chunk);
        friend class ConnectionImpl;
        std::vector<net::Chunk> _chunks{};
        std::size_t _chunksSize{0};
        UnorderedMap<String, CaseInsensitive> _headers{};
        CookieJar _cookies{};
        Status _status{http::Ok};
        Buffer _body{0};
        bool  _completed{false};
        ProtocolUpgrade  _upgrade{nullptr};
    };
}
}
#endif //SUIL_HTTP_SERVER_RESPONSE_HPP
