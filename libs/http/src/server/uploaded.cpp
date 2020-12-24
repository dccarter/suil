//
// Created by Mpho Mbotho on 2020-12-15.
//
#include "suil/http/server/uploaded.hpp"

#include <suil/base/file.hpp>

namespace suil::http::server {

    UploadedFile::UploadedFile(String name, Data data)
        : _name{std::move(name)},
          _data{std::move(data)}
    {}

    void UploadedFile::save(const String& dir, const Deadline& dd)
    {
        auto realDir = fs::realpath(dir());
        if (!realDir or !fs::isdir(realDir())) {
            throw InvalidArguments("directory '", dir, "' is invalid");
        }
        auto path = suil::catstr(realDir, "/", fs::filename(Ego._name));
        File writer{path, O_WRONLY | O_CREAT, 0644};
        writer.write(Ego._data.data(), Ego._data.size(), dd);
    }

    UploadedFile::operator bool() const
    {
        return !Ego._name.empty();
    }

}