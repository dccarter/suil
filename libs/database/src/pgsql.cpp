//
// Created by Mpho Mbotho on 2020-11-19.
//

#include "suil/db/pgsql.hpp"

namespace suil::db {

    PgSqlStatement::PgSqlStatement(PGconn *conn, String stmt, bool async, std::int64_t timeout)
        : conn{conn},
          stmt{std::move(stmt)},
          async{async},
          timeout{timeout}
    {}

    int PgSqlStatement::waitRead(int sock)
    {
        int events = fdwait(sock, FDW_IN, Deadline{timeout});
        if (events&FDW_ERR) {
            return -1;
        } else if (events&FDW_IN) {
            return 0;
        }
        errno = ETIMEDOUT;
        return ETIMEDOUT;
    }

    int PgSqlStatement::waitWrite(int sock) {

        int events = fdwait(sock, FDW_OUT, Deadline{timeout});
        if (events&FDW_ERR) {
            return -1;
        } else if (events&FDW_OUT) {
            return 0;
        }

        errno = ETIMEDOUT;
        return ETIMEDOUT;
    }

    PgSqlConnection::PgSqlConnection(PGconn *conn, String dbname, bool async, std::int64_t timeout, FreeConnFunc freeConn)
        : conn{conn},
          async{async},
          timeout{timeout},
          freeConn{std::move(freeConn)},
          dbname{std::move(dbname)}
    {}

    bool PgSqlConnection::hasTable(const String& name)
    {
        const String schema{"public"};
        return Ego.hasTable(schema, name);
    }

    bool PgSqlConnection::hasTable(const String& schema, const String& name)
    {
        auto stmt = Ego("SELECT COUNT(*) FROM pg_catalog.pg_tables WHERE schemaname=$1 and tablename=$2");
        int found{0};
        if (stmt(schema, name) >> found) {
            return found == 1;
        }
        return false;
    }

    PgSqlStatement PgSqlConnection::operator()(String req) {
        /* temporary zero copy string */
        itrace("%s", req());

        auto it = stmtCache.find(req);
        if (it != stmtCache.end()) {
            return it->second;
        }

        /* statement not cached, create new */
        auto tmp = req.peek();
        /* the key will be copied to statement so it won't be delete
         * when the statement is deleted */
        auto ret = stmtCache.insert(it,
                                     std::make_pair(std::move(req), PgSqlStatement(conn, std::move(tmp), async, timeout)));

        return ret->second;
    }

    PgSqlStatement PgSqlConnection::operator()(Buffer& req)
    {
        String tmp{req, false};
        itrace(PRIs, _PRIs(tmp));

        auto it = stmtCache.find(tmp);
        if (it != stmtCache.end()) {
            return it->second;
        }

        return (*this)(String{req});
    }

    void PgSqlConnection::destroy(bool dctor)
    {
        if (conn == nullptr || --refs > 0) {
            /* Connection still being used */
            return;
        }

        itrace("destroying Connection dctor %d %d", dctor, refs);
        if (async) {
            PGresult *res;
            while ((res = PQgetResult(conn)) != nullptr)
                PQclear(res);
        }

        if (freeConn) {
            /* call the function that will free the Connection */
            freeConn(this);
        }
        else {
            /* no function to free Connection, finish */
            PQfinish(conn);
        }
        conn = nullptr;

        if (!dctor) {
            deleting = true;
            /* if the call is not from destructor destroy*/
            delete this;
        }
    }

    PgSqlTransaction::PgSqlTransaction(PgSqlConnection& conn)
        : conn{conn}
    {}

    bool PgSqlTransaction::begin()
    {
        if (!Ego.valid) {
            Ego.valid = true;
            return Ego("BEGIN;");
        }
        return false;
    }

    bool PgSqlTransaction::commit()
    {
        if (Ego.valid) {
            Ego.valid = false;
            return Ego("COMMIT;");
        }
        return false;
    }

    bool PgSqlTransaction::rollback(const char *sp)
    {
        if (sp) {
            if (!Ego.valid) return false;
            return Ego("ROLLBACK TO SAVEPOINT $1;", sp);
        } else {
            if (Ego.valid) {
                Ego.valid = false;
                return Ego("ROLLBACK;");
            }
        }
        return false;
    }

    bool PgSqlTransaction::savepoint(const char *name)
    {
        if (Ego.valid) return Ego("SAVEPOINT $1;", name);
        return false;
    }

    bool PgSqlTransaction::release(const char *name)
    {
        if (Ego.valid) return Ego("RELEASE SAVEPOINT $1;", name);
        return false;
    }

    PgSqlTransaction::~PgSqlTransaction() noexcept
    {
        Ego.commit();
        conn.put();
    }

    PgSqlDb::~PgSqlDb()
    {
        if (cleaning) {
            /* unschedule the cleaning coroutine */
            itrace("notifying cleanup routine to exit");
            !notify;
        }

        itrace("cleaning up %lu connections", conns.size());
        auto it = conns.begin();
        while (it != conns.end()) {
            (*it).cleanup();
            conns.erase(it);
            it = conns.begin();
        }
    }

    PgSqlDb::Connection& PgSqlDb::connection()
    {
        PGconn *conn{nullptr};
        if (conns.empty()) {
            /* open a new Connection */
            int y{2};
            do {
                conn = open();
                if (conn) break;
                yield();
            } while (conns.empty() && y--);
        }

        // Still holding mutex
        if (conn == nullptr){
            // try find connection from cached list
            if(!conns.empty()){
                auto h = conns.back();
                /* cancel Connection expiry */
                h.alive = -1;
                conn = h.conn;
                conns.pop_back();
            }
            else {
                // connection limit reach
                throw PgSqlException("Postgres SQL connection limit reached");
            }
        }

        Connection *c = new Connection(
                conn, dbname.peek(), async, timeout,
                [&](Connection* _conn) {
                    free(_conn);
                });
        return *c;
    }

    PGconn* PgSqlDb::open()
    {
        PGconn *conn;
        conn = PQconnectdb(connectionStr());
        if (conn == nullptr || (PQstatus(conn) != CONNECTION_OK)) {
            itrace("CONNECT: %s", PQerrorMessage(conn));
            if (conn) PQfinish(conn);
            return nullptr;
        }

        if (async) {
            /* Connection should be set to non blocking */
            if (PQsetnonblocking(conn, 1)) {
                ierror("CONNECT: %s", PQerrorMessage(conn));
                throw PgSqlException("connecting to database failed");
            }
        }

        return conn;
    }

    void PgSqlDb::cleanup(PgSqlDb& db)
    {
        /* cleanup all expired connections */
        int64_t expires = db.keepAlive + 5;
        if (db.conns.empty())
            return;

        db.cleaning = true;

        do {
            /* if notified to exit, exit immediately*/
            uint8_t status{0};
            if ((db.notify[expires] >> status)) {
                if (status == 1) break;
            }

            /* was not forced to exit */
            auto it = db.conns.begin();
            /* un-register all expired connections and all that will expire in the
             * next 500 ms */
            int64_t t = mnow() + 500;
            int pruned = 0;
            ltrace(&db, "starting prune with %ld connections", db.conns.size());
            while (it != db.conns.end()) {
                if ((*it).alive <= t) {
                    lk.unlock();
                    (*it).cleanup();
                    lk.lock();
                    db.conns.erase(it);
                    it = db.conns.begin();
                } else {
                    /* there is no point in going forward */
                    break;
                }

                if ((++pruned % 100) == 0) {
                    /* avoid hogging the CPU */
                    yield();
                }
            }
            ltrace(&db, "pruned %ld connections", pruned);

            if (it != db.conns.end()) {
                /*ensure that this will run after at least 3 second*/
                expires = std::max((*it).alive - t, (int64_t)3000);
            }

        } while (!db.conns.empty());

        db.cleaning = false;
    }

    void PgSqlDb::free(Connection* conn) {
        conn_handle_t h {conn->conn, -1};

        if (keepAlive != 0) {
            /* set connections keep alive */
            h.alive = mnow() + keepAlive;
            conns.push_back(h);

            if (!cleaning && keepAlive > 0) {
                strace("scheduling cleaning...");
                /* schedule cleanup routine */
                go(cleanup(*this));
            }
        }
        else {
            /* cleanup now*/
            PQfinish(h.conn);
        }
    }
}