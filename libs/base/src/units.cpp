//
// Created by Mpho Mbotho on 2020-10-05.
//
#include "suil/base/units.hpp"

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

TEST_CASE("Byte size unit's conversion", "[bytes]") {
    REQUIRE(10_B == 10);
    REQUIRE(10_Kb == 10000);
    REQUIRE(10_Kib == 10240);
    REQUIRE(10_Mb == 10000000);
    REQUIRE(10_Mib == 10485760);
    REQUIRE(10_Gb == 10000000000);
    REQUIRE(10_Gib == 10737418240);
}

TEST_CASE("Time unit's conversion", "[time]") {
    REQUIRE(1000_us == 1);
    REQUIRE(15_ms == 15);
    REQUIRE(1_sec == 1000);
    REQUIRE(1_min == 60000);
    REQUIRE(2_hr == 7200000);
}
#endif