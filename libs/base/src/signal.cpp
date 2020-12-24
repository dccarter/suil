//
// Created by Mpho Mbotho on 2020-12-03.
//

#include "suil/base/signal.hpp"

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

using suil::Signal;
using suil::Connection;

TEST_CASE("Using suil signal/connection", "[signal]")
{
    WHEN("Using scoped connections to signals") {
        int count{0};
        Signal<void()> s1;

        // Scoped connection, this will go out of scope and get disconnected
        // immediately
        s1.connect([&](){
            // shouldn't be invoked because it will go out of scope immediately
            count++;
        });
        s1();
        REQUIRE(count == 0);
        {
            // this connection should go out of scope at the end of this
            // block
            auto conn = s1.connect([&]() {
                count++;
            });
            s1();
            REQUIRE(count == 1);
        }
        // there should be no connection registered with the signal
        s1();
        REQUIRE(count == 1);
        auto conn1 = s1.connect([&]() {
            count++;
        });
        s1();
        REQUIRE(count == 2);
        {
            // this connection should go out of scope at the end of this
            // block
            auto conn2 = s1.connect([&]() {
                count++;
            });
            s1();
            // conn1 and conn2 executed
            REQUIRE(count == 4);
        }
        s1();
        // Only conn1 left in scope
        REQUIRE(count == 5);
        conn1.disconnect();
        s1();
        // conn1 explicitly disconnected
        REQUIRE(count == 5);
    }

    WHEN("Using lifetime connections to signals") {
        Signal<void()> s1;
        int count{0};
        auto c1 = (s1 += [&] {
            count++;
        });
        s1();
        REQUIRE(count == 1);
        {
            // register a lifetime and forget
            s1 += [&] {
                count--;
            };
            s1();
            REQUIRE(count == 1);
        }
        s1();
        REQUIRE(count == 1);
        REQUIRE(c1);
        REQUIRE(s1.callbacks.size() == 2);
        REQUIRE(s1.lifeTimeObservers.size() == 2);
        // A lifetime connection need to be explicitly canceled
        s1.disconnect(c1);
        REQUIRE(s1.callbacks.size() == 2); // it will be unscheduled on next trigger
        REQUIRE(s1.lifeTimeObservers.size() == 1);
        REQUIRE_FALSE(c1);
        s1();
        REQUIRE(count == 0);
        REQUIRE(s1.callbacks.size() == 1);
    }
}

#endif