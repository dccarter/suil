//
// Created by Mpho Mbotho on 2020-10-16.
//

#include "suil/base/channel.hpp"

namespace suil {
    Void_t Void{};

    Sync::Sync()
        : sync(std::make_shared<Type>(Abort))
    {}

    Sync::~Sync() {
        if (active and sync) {
            !(*sync);
            sync = nullptr;
        }
    }

    Sync::Type& Sync::operator()() {
        return *sync;
    }

    Sync::Weak Sync::get()
    {
        return sync;
    }

    Conditional::~Conditional()
    {
        // abort all waiting channels
        notify(Sync::Abort);
    }

    bool Conditional::wait(Sync& sync, int64_t timeout)
    {
        if (sync.active) {
            throw ConditionalError("Cannot double wait a synchronous object");
        }

        waiting.insert(waiting.end(), sync.get());
        Sync::Result status{Sync::Abort};
        sync.active = true;
        if (sync()[timeout] >> status) {
            sync.active = false;
            return status == Sync::Activated;
        }
        sync.active = false;
        return false;
    }

    int Conditional::notify(Sync::Result res)
    {
        int notified{0};
        while (!waiting.empty()) {
            auto ptr = std::move(waiting.front());
            waiting.pop_front();
            if (auto ch = ptr.lock()) {
                // waiter still active
                if (res != Sync::Abort) {
                    (*ch) << res;
                }
                else {
                    !(*ch);
                }
                notified++;
            }
        }
        return notified;
    }

    bool Conditional::notifyOne()
    {
        if (!waiting.empty()) {
            auto ptr = std::move(waiting.front());
            waiting.pop_front();
            if (auto ch = ptr.lock()) {
                // waiter still active
                (*ch) << Sync::Activated;
                return true;
            }
        }
        return false;
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>


static coroutine void produce(suil::Conditional& cond, std::vector<int>& numbers)
{
    for (int i = 0; i < 20; i++) {
        numbers.push_back(i);
        if (!cond.notifyOne()) {
            yield();
        }
    }

    while (!numbers.empty()) {
        if (!cond.notifyOne()) {
            msleep(suil::Deadline{1000});
        }
    }
    cond.notify(suil::Sync::Abort);
}

static coroutine void consume(std::vector<int>& numbers, suil::Conditional& cond, suil::Sync& cs)
{
    while (cond.wait(cs)) {
        //printf("%p  -> %d\n", &cs, numbers.back());
        numbers.pop_back();
    }
}

TEST_CASE("Channel tests", "[common][Channel]") {
    // tests the Channel abstraction API
    SECTION("Constructor/Assignment test") {
        suil::Channel<bool> ch{suil::Void}; // creating a useless channel
        REQUIRE(ch.ch == nullptr);
        suil::Channel<bool> ch2{false};  // creating a useful channel
        REQUIRE_FALSE(ch2.ch == nullptr);
        REQUIRE(ch2.ddline == -1);
        REQUIRE(ch2.waitn == -1);
        suil::Channel<int> ch3{-10};  // creating a useful channel
        REQUIRE_FALSE(ch3.ch == nullptr);
        REQUIRE(ch3.term == -10);
        suil::Channel<int> ch4(std::move(ch3));
        REQUIRE(ch3.ch == nullptr);
        REQUIRE_FALSE(ch4.ch == nullptr);
        REQUIRE(ch4.term == -10);
        suil::Channel<int> ch5 = std::move(ch4);
        REQUIRE(ch4.ch == nullptr);
        REQUIRE_FALSE(ch5.ch == nullptr);
        REQUIRE(ch5.term == -10);

        struct Point {
            int  a;
            int  b;
        };
        suil::Channel<Point> ch6{Point{-1, 0}};
        REQUIRE_FALSE(ch6.ch == nullptr);
        REQUIRE(ch6.term.a == -1);
        REQUIRE(ch6.term.b == 0);
    }

    SECTION("Constructor/Assignment test") {
        suil::Conditional p;
        suil::Sync c1, c2;
        std::vector<int> nums;
        go(produce(p, nums));
        go(consume(nums, p, c1));
        go(consume(nums, p, c2));
        msleep(suil::Deadline{1000});
    }
}

#endif
