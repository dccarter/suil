//
// Created by Mpho Mbotho on 2020-11-20.
//

#include "suil/db/pgsql.hpp"
#include "suil/db/data.scc.hpp"

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
        db::PgSqlDb db1;
        const String connStr{"postgres://build:passwd@postgres:5432/dev"};
        db1.init(connStr,
                 opt(ASYNC, true), // connections are async
                 opt(TIMEOUT, 5000), // there is not timeout on operations
                 opt(EXPIRES, -1)  /* connections are not kept around */);
        {
            // create a users table
            scoped(conn, db1.connection());
            if (conn.hasTable("users")) {
                sdebug("database already has users table");
            } else {
                auto status = conn.createTable("users", db::User{});
                verify(status);
            }
        }

        {
            // Use SQL ORM to add data to database
            scoped(conn, db1.connection());
            db::Orm<db::PgSqlConnection, db::User> orm("users", conn);
            orm.truncate();
            verify(orm.insert(db::User{"lastcarter@gmail.com", "carter", 25, "+1-200-000-1234"}));
            int Id{0};
            verify(orm.selectColumn("id", Id, opt(Username, "carter")), "User carter should exist in table");

            db::User u1;
            verify(orm.find(opt(id, Id), u1), "user Carter should be successfully retrieved");
            {
                u1.Age += 1;
                verify(orm.update(u1), "Updating database should be allowed");
            }

            db::User u2{"lastcarter@dummy.com", "mmbotho", 23, "+1-200-000-1234"};
            u2.Key = "Random Key";
            verify(orm.insert(u2));
            db::User u3;
            verify(orm.find(opt(Username, "mmbotho"), u2), "user mmbotho should exist");
            verify(u3.Key.empty(), "key should be empty as it an ignored field");
        }
    }

    {
        db::PgSqlDb db1;
        const String connStr{"postgres://build:passwd@postgres:5432/dev"};
        std::uintptr_t uptr{0};
        db1.init(connStr,
                 opt(ASYNC, true), // connections are async
                 opt(TIMEOUT, 5000), // operations will fail if not serviced within 5 seconds
                 opt(EXPIRES, 30000)  /* Connections are kept alive for 30 seconds and re-used */);

        {
            scoped(conn, db1.connection());
            db::Orm<db::PgSqlConnection, db::User> orm("users", conn);
            auto users = orm.getAll();
            verify((users.size() == 2), "2 users should have been read from database");
            uptr = reinterpret_cast<std::uintptr_t>(&conn);
            auto results = conn("SELECT Username,Age FROM users WHERE Id=$")(0);
            std::tuple<std::string_view,int> res;
            results >> res;
        }
        yield();
        {
            scoped(conn, db1.connection());
            auto uptr2 = reinterpret_cast<std::uintptr_t>(&conn);
            verify((uptr == uptr2), "should have reused connection");
            db::Orm<db::PgSqlConnection, db::User> orm("users", conn);
            db::User u1;
            verify(orm.find(opt(Username, "mmbotho"), u1), "user mmbotho should exist");
            orm.remove(u1);
            verify(!orm.has(opt(Username, "mmbotho")), "user mmbotho should have been removed");
            orm.remove(opt(Username, "carter"));
            verify(!orm.has(opt(Username, "carter")), "user carter should have been removed");
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

