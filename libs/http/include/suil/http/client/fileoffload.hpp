//
// Created by Mpho Mbotho on 2020-12-29.
//

#ifndef SUIL_HTTP_CLIENT_FILEOFFLOAD_HPP
#define SUIL_HTTP_CLIENT_FILEOFFLOAD_HPP

#include <suil/http/client/response.hpp>

#include <suil/base/file.hpp>

namespace suil::http::cli {

    class FileOffload: public File {
    public:
        explicit FileOffload(String path, int64 timeout = -1);

        MOVE_CTOR(FileOffload) = default;
        MOVE_ASSIGN(FileOffload) = default;

        ~FileOffload() = default;

        inline Response::Writer& operator()() {
            return _handler;
        }

        bool operator()(const char *data, size_t len);

    private:
        DISABLE_COPY(FileOffload);
        int64 _timeout{-1};
        Response::Writer _handler{nullptr};
    };
}
#endif //SUIL_HTTP_CLIENT_FILEOFFLOAD_HPP
