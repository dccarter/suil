//
// Created by Mpho Mbotho on 2020-12-17.
//

#ifndef SUIL_HTTP_SERVER_OFFLOAD_HPP
#define SUIL_HTTP_SERVER_OFFLOAD_HPP

#include <suil/base/file.hpp>

namespace suil::http {

    class FileOffload : public File {
    public:
        using File::File;
        MOVE_CTOR(FileOffload);
        MOVE_ASSIGN(FileOffload);

        bool size(size_t& len) const;

        Data data();
        String data() const;

        void close() override;

        ~FileOffload() override;
    private:
        DISABLE_COPY(FileOffload);
        void  *_mapped{nullptr};
        size_t _size{0};
    };
}
#endif //SUIL_HTTP_SERVER_OFFLOAD_HPP
