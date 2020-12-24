//
// Created by Mpho Mbotho on 2020-12-15.
//

#ifndef SUIL_HTTP_SERVER_UPLOADFILE_HPP
#define SUIL_HTTP_SERVER_UPLOADFILE_HPP

#include <suil/base/string.hpp>

namespace suil::http::server {

    class UploadedFile {
    public:
        UploadedFile(String name, Data data);
        MOVE_CTOR(UploadedFile) = default;
        MOVE_ASSIGN(UploadedFile) = default;
        ~UploadedFile() = default;

        inline const String& name() const {
            return Ego._name;
        }

        inline const Data& data() const {
            return Ego._data;
        }

        operator bool() const;

        void save(const String& dir, const Deadline& dd = Deadline::Inf);

    private:
        friend class Form;
        DISABLE_COPY(UploadedFile);
        UploadedFile() = default;
        String _name{};
        Data   _data{};
    };
}
#endif //SUIL_HTTP_SERVER_UPLOADFILE_HPP
