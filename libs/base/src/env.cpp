//
// Created by Mpho Mbotho on 2020-10-08.
//

#include "suil/base/env.hpp"

namespace suil {

    const char* env(const char *name, const char *def) {
        const char *v = std::getenv(name);
        if (v != nullptr) {
            def = v;
        }
        return def;
    }

    String env(const char *name, const String& def) {
        const char *v = std::getenv(name);
        if (v != nullptr) {
            return String{v}.dup();
        }

        return def;
    }
}

#ifdef SUIL_UNITTEST
#include "catch2/catch.hpp"
#include "../test/test.hpp"

namespace sb = suil;

typedef decltype(iod::D(
        sb::test:: prop(a, int),
        sb::test:: prop(b, sb::String),
        sb::test:: prop(c, float)
)) Config;

TEST_CASE("Using env API's", "[env][encvonfig]") {
    WHEN("Loading environment variables") {
        REQUIRE_FALSE(sb::env("PATH", sb::String{}).empty());
        setenv("INT", "5", true);
        setenv("DOUBLE", "5.7", true);
        setenv("BOOL", "true", true);
        setenv("STRING", "Hello", true);

        REQUIRE(sb::env("INT", int(0)) == 5);
        REQUIRE(sb::env("DOUBLE", double(0)) == 5.7);
        REQUIRE(sb::env("BOOL", bool(false)));
        REQUIRE(sb::env("STRING", std::string{}) == "Hello");
        REQUIRE(sb::env("STRING", sb::String{}) == "Hello");
        REQUIRE(strcmp(sb::env("STRING", (const char*)nullptr), "Hello") == 0);

        REQUIRE(sb::env("DUMMY", sb::String{"One"}) == "One");
        REQUIRE(sb::env("DUMMY", bool(true)));
    }

    WHEN("Loading configuration from environment variables") {
        setenv("A", "1", true);
        setenv("B", "Hello", true);
        setenv("C", "6.92", true);
        Config config{0, "", 0};
        sb::envconfig(config);
        REQUIRE(config.a == 1);
        REQUIRE(config.b == "Hello");
        REQUIRE(config.c == 6.92f);
    }
}
#endif
