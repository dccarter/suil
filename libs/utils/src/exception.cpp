/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-17
 */

#include "suil/utils/exception.hpp"

namespace suil {

    static const std::string ExceptionTag{"Exception"};

    Exception::Exception(std::string message)
            : mTag{ExceptionTag},
              mId(makeId(ExceptionTag)),
              mMessage{std::move(message)}
    {}

    Exception Exception::fromCurrent() {
        auto ex = std::current_exception();
        if (ex == nullptr) {
            // Exception::fromCurrent cannot be called outside a catch block
            throw  Exception{"Exception::fromCurrent cannot be called outside a catch block"};
        }

        try {
            // rethrown exception
            std::rethrow_exception(ex);
        }
        catch (const Exception& ex) {
            // catch as current and return copy
            return Exception(ex.tag(), ex.id(), ex.message());
        }
        catch (const std::exception& e) {
            // catch and wrap in Exception class
            return Exception(std::string(e.what()));
        }
    }

    std::string Exception::makeTag(const std::string_view &sv)
    {
        auto pos = sv.find_last_of("::Tag");
        if (pos == std::string_view::npos) {
            return std::string{sv};
        }
        std::size_t start = sizeofcstr("static const string& ");
        std::size_t count = pos - start - sizeofcstr("::Tag") + 1;
        return std::string{sv.substr(start, count)};
    }

    std::uint64_t Exception::makeId(const std::string &tag)
    {
        return std::hash<std::string>{}(tag);
    }
}

#ifdef SXY_UNITTEST

#include "catch2/catch.hpp"
DECLARE_EXCEPTION(CustomException);

TEST_CASE("Throwing exceptions", "[Exception]") {
    WHEN("Using exception properties") {
        suil::Exception ex("Hello");
        REQUIRE(ex.tag() == suil::ExceptionTag);
        REQUIRE(ex.id()  == std::hash<std::string>{}(suil::ExceptionTag));
        REQUIRE(ex.message() == "Hello");

        REQUIRE(suil::AccessViolation::Tag() == "suil::AccessViolation");
        suil::AccessViolation av("Hello");
        REQUIRE(av.tag() == suil::AccessViolation::Tag());
        REQUIRE(av.id() == suil::AccessViolation::Id());
        REQUIRE(ex.message() == "Hello");

        REQUIRE(CustomException::Tag() == "CustomException");
        CustomException cex;
        REQUIRE(cex.id() == CustomException::Id());

        REQUIRE(CustomException{} != suil::AccessViolation{});
    }

    WHEN("Handling suil exceptions") {
        try {
            throw suil::AccessViolation();
        }
        catch (const suil::Exception& ex) {
            REQUIRE(ex.id() == suil::AccessViolation::Id());
            REQUIRE(ex.tag() == suil::AccessViolation::Tag());
            REQUIRE(ex == suil::AccessViolation{});
        }
    }
}

#endif

