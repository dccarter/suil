//
// Created by Mpho Mbotho on 2020-11-23.
//

#ifndef SUILNETWORK_RESPONSE_HPP
#define SUILNETWORK_RESPONSE_HPP

#include <suil/base/string.hpp>

#include <vector>

namespace suil::net::smtp {

    class Response {
    public:
        DISABLE_COPY(Response);

        MOVE_CTOR(Response) = default;

        MOVE_ASSIGN(Response) = default;

        static constexpr int AbortCode{0};

        template<typename ...Lines>
        static Response abort(int code, Lines&& ... lines) {
            Response res(code, std::forward<Lines>(lines)...);
            res.mIsAbort = true;
            return std::move(res);
        }

        template<typename ...Lines>
        static Response create(int code, Lines&& ... lines) {
            Response res(code, std::forward<Lines>(lines)...);
            return std::move(res);
        }

        inline void push(String line) {
            mLines.push_back(std::move(line));
        }

        inline const std::vector<String>& lines() const {
            return mLines;
        }

        inline bool isAbort() const {
            return mIsAbort;
        }

        inline int code() const {
            return mCode;
        }

        Response()
            : mIsAbort{true}
        {}

    private:
        template<typename... Lines>
        Response(int code, Lines&&... lines)
                : mCode{code}
        {
            addLines(std::forward<Lines>(lines)...);
        }

        template <typename ...Lines>
        void addLines(String line, Lines&&... lines) {
            mLines.push_back(std::move(line));
            if constexpr (sizeof...(lines) > 0) {
                addLines(std::forward<Lines>(lines)...);
            }
        }

    private:
        int mCode;
        std::vector<String> mLines;
        bool mIsAbort{false};
    };

}
#endif //SUILNETWORK_RESPONSE_HPP
