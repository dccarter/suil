//
// Created by Mpho Mbotho on 2020-12-24.
//

#ifndef SUIL_HTTP_CLIENT_FORM_HPP
#define SUIL_HTTP_CLIENT_FORM_HPP

#include <suil/base/base64.hpp>
#include <suil/base/buffer.hpp>
#include <suil/base/file.hpp>

namespace suil::http::cli {

    class Form {
    public:
        static constexpr int UrlEncoded{0};
        static constexpr int MultipartForm{1};
        static constexpr int MultipartOther{2};

        struct File {
            File(String path, String ctype = nullptr);

            MOVE_CTOR(File) = default;
            MOVE_ASSIGN(File) = default;
            DISABLE_COPY(File);

            String name{};
            String path{};
            String contentType{};
            std::size_t size{0};
        };

        Form() = default;

        template <typename... Params>
        Form(int encoding, Params&&... params)
            : _encoding{encoding},
              _boundary{suil::catstr("----", suil::randbytes(8))}
        {
            Ego.appendArgs(std::forward<Params>(params)...);
        }

        template <typename... Params>
        Form(Params&&... params)
            : Form(UrlEncoded, std::forward<Params>(params)...)
        {}

        MOVE_CTOR(Form) noexcept;
        MOVE_ASSIGN(Form) noexcept;

        ~Form();

        inline operator bool() const {
            return !Ego._data.empty() or !Ego._files.empty();
        }

        std::size_t encode(Buffer& dst) const;

        void clear();

    private:
        DISABLE_COPY(Form);
        std::size_t encodeUrlEncoded(Buffer& dst) const;
        std::size_t encodeMultipart(Buffer& dst) const;

        void appendArg(String name, String val) {
            if (Ego._encoding != UrlEncoded) {
                Ego._data.emplace(
                        std::move(name), std::move(val));
            }
            else {
                Ego._data.emplace(
                        Base64::urlEncode(name),
                        Base64::urlEncode(val));
            }
        }

        template <typename V>
            requires std::is_arithmetic_v<V>
        void appendArg(String name, V&& val) {
            Ego.appendArg(std::move(name), suil::tostr(val));
        }

        template <typename V>
            requires std::is_same_v<V, Form::File>
        void appendArg(String name, V&& val) {
            val.name = std::move(name);
            val.size = suil::fs::size(val.path());
            Ego._files.push_back(std::forward<V>(val));
            _encoding = MultipartForm;
        }

        template <typename Arg, typename... Args>
        void appendArgs(String name, Arg&& arg, Args&&... args) {
            Ego.appendArg(std::move(name), std::forward<Arg>(arg));
            if constexpr (sizeof...(args) > 0) {
                Ego.appendArgs(std::forward<Args>(args)...);
            }
        }


        struct Upload {
            Upload(File const* file, String head);
            MOVE_CTOR(Upload) noexcept;
            MOVE_ASSIGN(Upload) noexcept;

            int getFd() const {
                return Ego._fd;
            }

            const String& getHead() const {
                return Ego._head;
            }

            size_t size() const {
                return _file == nullptr? 0 : _file->size;
            }

            int open();
            void close();
            ~Upload();
        private:
            DISABLE_COPY(Upload);
            File const* _file{nullptr};
            String _head{};
            int    _fd{-1};
        };

        friend class Request;

        String _boundary{};
        int    _encoding{UrlEncoded};
        std::vector<File> _files{};
        mutable std::vector<Upload> _uploads{};
        mutable UnorderedMap<String> _data{};
    };

    using UpFile = Form::File;
}
#endif //SUIL_HTTP_CLIENT_FORM_HPP
