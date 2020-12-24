//
// Created by Mpho Mbotho on 2020-10-08.
//

#include "suil/base/regex.hpp"

namespace suil::regex {

    bool match(std::regex& reg, const char *data, size_t len, regex::Flags flags) {
        using svmatch = std::match_results<std::string_view::const_iterator>;
        using svsub_match = std::sub_match<std::string_view::const_iterator>;

        std::string_view sv{data, len};
        return std::regex_match(sv.begin(), sv.end(), reg, flags);
    }
}