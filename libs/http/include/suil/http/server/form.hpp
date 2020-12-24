//
// Created by Mpho Mbotho on 2020-12-15.
//

#ifndef LIBS_HTTP_INCLUDE_SUIL_HTTP_SERVER_FORM_HPP
#define LIBS_HTTP_INCLUDE_SUIL_HTTP_SERVER_FORM_HPP

#include <suil/http/server/uploaded.hpp>

#include <suil/base/buffer.hpp>
#include <suil/base/sio.hpp>

#include <set>

namespace suil::http::server {

    class Form {
    public:
        using UploadEnumerator = std::function<bool(const UploadedFile&)>;
        using ParamsEnumerator = std::function<bool(const String&, const String&)>;

        const UploadedFile& getUpload(const String& name) const;
        const String& get(const String& name) const;
        void operator|(UploadEnumerator enumerator);
        void operator|(ParamsEnumerator enumerator);
        const UnorderedMap<String>& params() const;
        const UnorderedMap<UploadedFile>& uploads() const;
        const String& operator[](const String& name) const;
        const UploadedFile& operator()(const String& name) const;

        template <typename T>
            requires (std::is_base_of_v<iod::MetaType, T> or std::is_base_of_v<iod::UnionType, T>)
        String read(T& o, const std::set<String>& required = {}, const String& sep = "\n") const {
            Buffer ob;
            iod::foreach2(T::Meta)| [&](const auto& m) {
                String name{m.symbol().name()};
                auto isRequired = required.find(name) != required.end();
                auto it = Ego._params.find(name);
                if (it == Ego._params.end()) {
                    auto& value = it->second;
                    if (isRequired and value.empty()) {
                        ob << (ob.empty()? "": sep) << "Required field '" << name << "' cannot be empty";
                    }
                    else {
                        try {
                            suil::cast(value, m.symbol().member_access(o));
                        }
                        catch (...) {
                            ob << (ob.empty()? "": sep) << "Field '" << name << "' has unsupported value";
                        }
                    }
                }
                else if (isRequired) {
                    ob << (ob.empty()? "": sep) << "Required field '" << name << "' must be provided";
                }
            };

            return String{ob};
        }

        template <typename T>
        bool get(const String& name, T& o) {
            auto val = get(name);
            if (val.empty()) {
                return false;
            }

            suil::cast(val, o);
            return true;
        }

        void clear();

    private:
        friend class Request;
        Form() = default;
        void add(String name, UploadedFile upload);
        void add(String name, String value);
        UnorderedMap<UploadedFile> _uploads{};
        UnorderedMap<String> _params{};
    };
}
#endif //LIBS_HTTP_INCLUDE_SUIL_HTTP_SERVER_FORM_HPP
