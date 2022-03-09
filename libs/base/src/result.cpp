//
// Created by Mpho Mbotho on 2020-10-24.
//

#include "suil/base/result.hpp"

namespace suil {

    Result::Result()
        : Buffer()
    {}

    Result& Result::operator=(Result&& other) noexcept
    {
        if (&other != this) {
            Buffer::operator=(std::move(other));
            Code = other.Code;
        }
        return Ego;
    }

    Result::Result(Result&& other) noexcept
        : Buffer(std::move(other)),
          Code{other.Code}
    {}

    bool Result::Ok() const {
        return Code == Result::OK;
    }

    Result& Result::operator()(int code)
    {
        if (code != Result::OK) {
            if (!Ego.empty()) {
                Ego << '\n';
            }
            Ego << "Code_" << code << ": ";
            Ego.Code = code;
        }
        else {
            Ego << "\n\t";
        }
        return Ego;
    }

    Result & Result::operator()()
    {
        Ego << "\n\t";
        return Ego;
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

using suil::Result;

TEST_CASE("Using Result API's", "[Result]") {
    WHEN("Returning a pass/fail result") {
        Result result{};
        REQUIRE(result.Ok());
        REQUIRE(result.Code == Result::OK);

        REQUIRE_FALSE(Result{Result::Error}.Ok());
        REQUIRE_FALSE(Result{1}.Ok());
        REQUIRE_FALSE(Result{-20}.Ok());
        REQUIRE(Result{5}.Code == 5);

        Result res1{Result::Error, "Simple error message"};
        REQUIRE_FALSE(res1.Ok());
        REQUIRE(suil::std::string_view(res1) == "Simple error message");
    }

    WHEN("Building stacked results") {
        Result res;
        REQUIRE(res.Ok());
        res << "Good status code";
        REQUIRE(res.Ok());
        REQUIRE(suil::std::string_view(res) == "Good status code");
        res();
        REQUIRE(res.Ok());
        REQUIRE(suil::std::string_view(res) == "Good status code\n\t");
        res << "Other good status code";
        REQUIRE(res.Ok());
        REQUIRE(suil::std::string_view(res) == "Good status code\n\tOther good status code");
        res(Result::Error);
        REQUIRE_FALSE(res.Ok());
        REQUIRE(suil::std::string_view(res) == "Good status code\n\tOther good status code\nCode_-1: ");
        res << "Error stuff";
        REQUIRE_FALSE(res.Ok());
        REQUIRE(suil::std::string_view(res) == "Good status code\n\tOther good status code\nCode_-1: Error stuff");
    }
}
#endif
