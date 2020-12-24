//
// Created by Mpho Mbotho on 2020-11-27.
//

#ifndef SUILNETWORK_UTILS_HPP
#define SUILNETWORK_UTILS_HPP

#include <suil/base/string.hpp>

#include <set>

namespace suil::net::smtp {

    /**
     * Compute the code associate with the given character array \param p
     * at compile time
     * @param p the 4 characters whose code should be computed
     * @return the computed code
     */
    constexpr int code(const char p[4]) {
        return (int(p[0])<<24) | (int(p[1])<<16) | (int(p[2])<<8) | int(p[3]);
    }

    /**
     * Compute the code associate with the given character array \param p
     * at runtime
     * @param p the 4 characters whose code should be computed
     * @param caps true to convert the characters to uppercase
     * @return the computed code
     */
    int code_r(const char p[4], bool igc = true);

    /**
     * Capture all characters in the given string \param s starting at index \param idx
     * until the given function \param until returns true
     * @param s the string to capture from
     * @param idx the index at which the capture will be started
     * @param until the function that will terminate the capture
     * @return the captured string if any
     */
    String capture(const String& s, int idx, std::function<bool(char)> until = isspace);

    /**
     * Traverse parameter \param idx until the function \param when returns false
     * @param s the string to check
     * @param idx the index to start checking at and to traverse
     * @param when the function to terminate the eating!
     * @return
     */
    String eat(const String& s, int idx, std::function<bool(char)> when = isspace);

    inline std::function<bool(char)> isChar(char c, bool igc = false) {
        return [c,igc](char x) {
            return igc? toupper(x) == toupper(c): x == c;
        };
    }

    inline std::function<bool(char)> allButChar(char c, bool igc = false) {
        return [c,igc](char x) {
            return igc? (::toupper(x) != ::toupper(c)) : (x != c);
        };
    }

    inline std::function<bool(char)> anyOf(const std::set<char>& chs) {
        return [&chs](char x) {
            return chs.find(x) != chs.end();
        };
    }

    inline std::function<bool(char)> noneOf(const std::set<char>& chs) {
        return [&chs](char x) {
            return chs.find(x) == chs.end();
        };
    }
}
#endif //SUILNETWORK_UTILS_HPP
