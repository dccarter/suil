//
// Created by Mpho Mbotho on 2020-11-27.
//

#include "suil/net/smtp/common.hpp"

#include <cctype>

namespace suil::net::smtp {

    int code_r(const char p[4], bool caps)
    {
        if (caps) {
            return
                    (::toupper(p[0])<<24) |
                    (::toupper(p[1])<<16) |
                    (::toupper(p[2])<<8)  |
                    p[3];
        }
        return (int(p[0]) <<24) | (int(p[1])<<16) | (int(p[2])<<8) | int(p[3]);
    }

    String capture(const String& s, int idx, std::function<bool(char)> until)
    {
        auto it{idx};
        // traverse while loop until until haha
        while ((it < s.size()) and !until(s[it])) it++;
        if (it == idx) {
            return String{};
        }
        auto tmp = (it < s.size())? s.substr(idx, (it-idx)) : String{};
        return tmp;
    }

    String eat(const String& s, int idx,std::function<bool(char)> when)
    {
        // traverse while loop until until haha
        while ((idx < s.size()) and when(s[idx])) idx++;
        auto tmp = s.substr(idx);
        return tmp;
    }
}