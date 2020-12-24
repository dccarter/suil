//
// Created by Mpho Mbotho on 2020-10-24.
//

#ifndef SUIL_BASE_RESULT_HPP
#define SUIL_BASE_RESULT_HPP

#include "suil/base/buffer.hpp"

namespace suil {

    class Result : public Buffer {
    public:
        enum : int {
            OK = 0,
            Error = -1
        };

        Result();

        template<typename... Args>
        Result(int code, Args... args) noexcept
            : Buffer(),
              Code{code}
        {
            (Ego << ... << args);
        }

        Result(const Result&) = delete;
        Result& operator=(const Result&) = delete;

        Result(Result&& other) noexcept;
        Result& operator=(Result&& other) noexcept;

        bool Ok() const;

        Result& operator()(int code);

        Result& operator()();

        int Code{OK};
    };
}
#endif //SUIL_BASE_RESULT_H
