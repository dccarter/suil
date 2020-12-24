//
// Created by Mpho Mbotho on 2020-10-08.
//

#ifndef SUIL_BASE_REGEX_HPP
#define SUIL_BASE_REGEX_HPP

#include "suil/base/string.hpp"
#include "suil/base/buffer.hpp"

#include <regex>

namespace suil::regex {
    using Flags = std::regex_constants::match_flag_type;
    namespace Constants = std::regex_constants;

    /**
     * Check if the given data matches the given regular expression
     *
     * @param reg the regular expression to match against
     * @param data buffer with data to match against
     * @param len the size of the buffer
     * @param flags flags to use when matching
     * @return true if the given data matches the given regex
     */
    bool match(std::regex& reg, const char *data, size_t len, regex::Flags flags = Constants::match_default);

    /**
     *  Check if the given string matches the given regular expression
     *
     * @param rstr regular expression string
     * @param data the string to test
     * @param len (default: 0) the size of the string, if not specified or 0,
     *  then the string will be computed from from the input buffer
     * @return true if the string matches the given regular expression
     */
    inline bool match(const char *rstr, const char *data, size_t len = 0) {
        if (len == 0) len = strlen(data);
        std::regex reg(rstr);
        return match(reg, data, len);
    }

    /**
     * Matches the given data against the given regular expression string
     *
     * @tparam T DataBuf concept
     * @param rstr the regular expression string
     * @param data the data to test against the regular expression
     * @return true if the given data matches the given regular expression
     */
    template <DataBuf T>
    inline bool match(const char *rstr, const T& data) {
        std::regex reg(rstr);
        return match(reg, data.data(), data.size());
    }
}

#endif //SUIL_BASE_REGEX_HPP
