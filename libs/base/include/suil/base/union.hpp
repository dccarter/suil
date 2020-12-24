//
// Created by Mpho Mbotho on 2020-11-06.
//

#ifndef SUILBASE_UNION_HPP
#define SUILBASE_UNION_HPP

#include <type_traits>
#include <utility>
#include <variant>

namespace suil {

    template <typename T, int I = 0>
    class UnionMember {
    public:
        static constexpr int UnionIndex = I;

        template<typename... Args>
        UnionMember(Args... args)
            : Value{std::forward<Args>(args)...}
        {}

        template<typename U = T>
            requires std::is_copy_constructible_v<U>
        UnionMember(const UnionMember& u)
            : Value(u.Value)
        {}

        template<typename U = T>
            requires std::is_copy_assignable_v<U>
        UnionMember& operator=(const UnionMember& u) {
            Value = u.Value;
            return *this;
        }

        template<typename U = T>
        requires std::is_move_constructible_v<U>
        UnionMember(UnionMember&& u)
            : Value(std::move(u.Value))
        {}

        template<typename U = T>
        requires std::is_move_assignable_v<U>
        UnionMember& operator=(UnionMember&& u) {
            if (this != &u) {
                Value = std::move(u.Value);
            }
            return *this;
        }

        operator T&() { return Value; }
        operator const T&() const { return Value; }
        T& operator*() { return  Value; }
        const T& operator*() const { return Value; }

        T Value;
    };
}
#endif //SUILBASE_UNION_HPP
