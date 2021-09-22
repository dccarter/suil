//
// Created by Mpho Mbotho on 2020-12-14.
//

#ifndef SUIL_HTTP_SERVER_QS_HPP
#define SUIL_HTTP_SERVER_QS_HPP

#include <suil/base/string.hpp>
#include <suil/base/exception.hpp>

namespace suil::http::server {

    class QueryString {
    public:
        static const int MaxArguments{256};
        QueryString() = default;
        QueryString(const String& sv);
        MOVE_CTOR(QueryString);
        MOVE_ASSIGN(QueryString);

        void clear();
        String get(const String& name) const;
        std::vector<String> getAll(const String& name) const;

        template <typename T>
        Return<T> get(const String& name) const {
            auto str = get(name);
            return TryCatch<T>([&]() -> Return<T> {
                if (!str.empty()) {
                    T tmp{};
                    suil::cast(str, tmp);
                    return tmp;
                }
                return KeyNotFound{"Query string does not contain key '", name, "'"};
            });
        }

    private suil_ut:
        char *_url{nullptr};
        std::vector<char *> _params;
    };
}
#endif //SUIL_HTTP_SERVER_QS_HPP
