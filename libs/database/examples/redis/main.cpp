//
// Created by Mpho Mbotho on 2020-11-21.
//

//
// Created by Mpho Mbotho on 2020-11-20.
//

#include "suil/db/redis.hpp"

#include <iostream>

namespace db = suil::db;
using suil::String;

template <typename ...Args>
void _verify(bool val, Args... args)
{
    if (!val) {
        (std::cerr << ... << args) << std::endl;
        exit(EXIT_FAILURE);
    }
}

#define verify(op, ...) _verify((op), __FILE__, ":", __LINE__, " ", #op, " ",  ##__VA_ARGS__)

void start()
{
    {
        db::RedisDb db1("redis", 6379, db::RedisDbConfig{});
        const auto& info = db1.getServerInfo();
        sdebug("RedisDb version: " PRIs, _PRIs(info.Version));
        {
            scoped(conn, db1.connect());
            verify(conn.set("Username", "Carter"), "setting value should succeed");
            verify(conn.exists("Username"), "key Username must exist");
            verify((conn.get<String>("Username") == "Carter"), "read key must be Carter");
            conn.expire("Username", 4);
            msleep(suil::Deadline{6000});
            auto ttl = conn.ttl("Username");
            verify(!conn.exists("Username"), "Key should have expired");
        }
    }
}

int main(int argc, char *argv[])
{
    try {
        start();
    }
    catch (...) {
        auto ex = suil::Exception::fromCurrent();
        verify(false, "unhandled exception: ", ex.what());
    }
}

