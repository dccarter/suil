//
// Created by Mpho Mbotho on 2020-12-15.
//

#include "suil/http/server/form.hpp"

namespace suil::http::server {

    const UnorderedMap<String> & Form::params() const {
        return Ego._params;
    }

    const UnorderedMap<UploadedFile> & Form::uploads() const {
        return Ego._uploads;
    }

    const String& Form::get(const String& name) const {
        auto it = Ego._params.find(name);
        if (it == Ego._params.end()) {
            static String Invalid{};
            return Invalid;
        }
        return it->second;
    }

    const UploadedFile& Form::getUpload(const String& name) const
    {
        auto it = Ego._uploads.find(name);
        if (it == Ego._uploads.end()) {
            static UploadedFile Invalid{};
            return Invalid;
        }
        return it->second;
    }

    const UploadedFile& Form::operator()(const String& name) const
    {
        return getUpload(name);
    }

    const String& Form::operator[](const String& name) const
    {
        return get(name);
    }

    void Form::operator|(ParamsEnumerator enumerator)
    {
        for (auto& [key, name]: Ego._params) {
            if (!enumerator(key, name)) {
                break;
            }
        }
    }

    void Form::operator|(UploadEnumerator enumerator)
    {
        for (auto& [_, up]: Ego._uploads) {
            if (!enumerator(up)) {
                break;
            }
        }
    }

    void Form::add(String name, UploadedFile upload)
    {
        Ego._uploads.emplace(std::move(name), std::move(upload));
    }

    void Form::add(String name, String value)
    {
        Ego._params.emplace(std::move(name), std::move(value));
    }

    void Form::clear()
    {
        Ego._params.clear();
        Ego._uploads.clear();
    }
}