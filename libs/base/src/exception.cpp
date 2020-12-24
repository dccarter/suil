//
// Created by Mpho Mbotho on 2020-10-05.
//

#include "suil/base/exception.hpp"

#include <functional>

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

#ifdef SUIL_UNITTEST
#include "catch2/catch.hpp"
namespace sb = suil;
DECLARE_EXCEPTION(CustomException);

TEST_CASE("Throwing exceptions", "[Exception]") {
    WHEN("Using exception properties") {
        sb::Exception ex("Hello");
        REQUIRE(ex.tag() == sb::ExceptionTag);
        REQUIRE(ex.id()  == std::hash<std::string>{}(sb::ExceptionTag));
        REQUIRE(ex.message() == "Hello");

        REQUIRE(sb::AccessViolation::Tag() == "suil::AccessViolation");
        sb::AccessViolation av("Hello");
        REQUIRE(av.tag() == sb::AccessViolation::Tag());
        REQUIRE(av.id() == sb::AccessViolation::Id());
        REQUIRE(ex.message() == "Hello");

        REQUIRE(CustomException::Tag() == "CustomException");
        CustomException cex;
        REQUIRE(cex.id() == CustomException::Id());

        REQUIRE(CustomException{} != sb::AccessViolation{});
    }

    WHEN("Handling suil exceptions") {
        try {
            throw sb::AccessViolation();
        }
        catch (const sb::Exception& ex) {
            REQUIRE(ex.id() == sb::AccessViolation::Id());
            REQUIRE(ex.tag() == sb::AccessViolation::Tag());
            REQUIRE(ex == sb::AccessViolation{});
        }
    }
}

#endif