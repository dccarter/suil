//
// Created by Mpho Mbotho on 2020-12-29.
//

#ifndef SUIL_HTTP_CLIENT_REQUEST_HPP
#define SUIL_HTTP_CLIENT_REQUEST_HPP

#include <suil/http/client/form.hpp>
#include <suil/http/common.hpp>

#include <suil/net/socket.hpp>
#include <suil/base/json.hpp>

namespace suil::http::cli {

    define_log_tag(HTTP_CLIENT);
    class Request : LOGGER(HTTP_CLIENT) {
    public:
        using Builder = std::function<bool(Request&)>;

        MOVE_CTOR(Request) = default;
        MOVE_ASSIGN(Request) = default;

        void header(String name, String value);

        template <typename V>
        inline void header(String name, V&& val) {
            header(std::move(name), suil::tostr(val));
        }

        template <typename V, typename... Headers>
        void headers(String name, V&& val, Headers&&... hs) {
            header(std::move(name), std::forward<V>(val));
            if constexpr (sizeof... (hs) > 0) {
                headers(std::forward<Headers>(hs)...);
            }
        }

        void arg(String name, String value);

        template <typename V>
        inline void arg(String name, V&& val) {
            arg(std::move(name), suil::tostr(val));
        }

        template <typename V, typename... Args>
        void args(String name, V&& val, Args&&... args) {
            arg(std::move(name), std::forward<V>(val));
            if constexpr (sizeof... (args) > 0) {
                headers(std::forward<Args>(args)...);
            }
        }

        void setKeepAlive(bool on = true);

        Request& operator<<(Form&& form);
        Request& operator<<(File&& body);

        template <typename T>
            requires (iod::IsUnionType<T> or iod::IsMetaType<T> or std::is_same_v<T, json::Object>)
        inline Request& operator<<(const T& val) {
            return (Ego("application/json") << json::encode(val));
        }

        template <typename T>
        inline Request& operator<<(const T& val) {
            return (Ego() << val);
        }

        Buffer& operator()(String contentType = "text/plain");

        uint8* buffer(size_t size, String contentType = "text/plain");

        ~Request() override;

    private:
        DISABLE_COPY(Request);

        explicit Request(net::Socket::UPtr sock);

        void reset(Method m, String res, bool clear = true);
        void cleanup();
        void encodeArgs(Buffer& dst) const;
        void encodeHeaders(Buffer& dst) const;
        size_t buildBody();
        void submit(int64 timeout = -1);

        friend class Session;
        net::Socket::UPtr _sock{nullptr};
        UnorderedMap<String, CaseInsensitive> _headers;
        UnorderedMap<String> _arguments;
        Method _method{Method::Unknown};
        String _resource{};
        Form _form{};
        Buffer _body{};
        File _bodyFd{nullptr};
    };
}
#endif //SUIL_HTTP_CLIENT_REQUEST_HPP
