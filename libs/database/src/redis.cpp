//
// Created by Mpho Mbotho on 2020-11-19.
//

#include "suil/db/redis.hpp"

#include <suil/net/ssl.hpp>
#include <suil/net/tcp.hpp>

namespace suil::db {
    static RedisDbConfig redisDefaultConfig;

    String RedisClient::Command::operator()() const
    {
        return String(buffer.data(), std::min(size_t(64), buffer.size()), false).dup();
    }

    void RedisClient::Command::addparam(const String& param)
    {
        buffer << PrefixString << param.size() << REDIS_CRLF;
        buffer << param << REDIS_CRLF;
    }

    void RedisClient::Command::addHexParam(const void *data, size_t size)
    {
        size_t  sz =  size << 1;
        buffer << PrefixString << sz << REDIS_CRLF;
        buffer.reserve(sz);
        sz = suil::hexstr(static_cast<const uint8_t*>(data), size,
                           &buffer.data()[buffer.size()], buffer.capacity());
        buffer.seek(sz);
        buffer << REDIS_CRLF;
    }

    RedisClient::Reply::Reply(char prefix, String data)
        : prefix{prefix},
          data{std::move(data)}
    {}

    RedisClient::Reply::Reply(Buffer rxb)
        : received{std::move(rxb)},
          prefix{rxb.data()[0]},
          data{&rxb.data()[1], rxb.size()-3, false}
    {
        // null terminate
        data.data()[data.size()] = '\0';
    }

    RedisClient::Reply::operator bool() const
    {
        if (prefix == PrefixError) {
            return false;
        }

        return (prefix == PrefixArray) || !data.empty();
    }

    bool RedisClient::Reply::status(const char *expect) const
    {
        return (prefix == PrefixValue) and (data.compare(expect) == 0);
    }

    const char * RedisClient::Reply::error() const
    {
        if (prefix == PrefixError) {
            return data();
        }
        return "";
    }

    bool RedisClient::Reply::special() const
    {
        return (prefix == '[') or (prefix == ']');
    }

    RedisClient::Response::Response(Reply rp)
    {
        entries.push_back(std::move(rp));
    }

    RedisClient::Response::operator bool() const
    {
        if (entries.empty()) {
            return false;
        }
        if (entries.size() > 1) {
            return true;
        }
        return entries[0];
    }

    const char * RedisClient::Response::error() const
    {
        if (entries.empty()) {
            return "";
        }

        if (entries.size() > 1) {
            return entries.back().error();
        }

        return entries.front().error();
    }

    bool RedisClient::Response::status(const char *expect) const
    {
        return not(entries.empty()) and entries.front().status(expect);
    }

    String RedisClient::Response::peek(int index) const
    {
        if (index >= entries.size()) {
            throw RedisDbError("index '", index, "' out of range '", entries.size(), "'");
        }

        return entries[index].data.peek();
    }

    void RedisClient::Response::castreply(int index, String& d) const
    {
        d = entries[index].data.dup();
    }

    const String& RedisClient::ServerInfo::operator[](const String& key) const
    {
        static const String NotFound{};
        auto it = params.find(key);
        if (it != params.end()) {
            return it->second;
        }
        return NotFound;
    }

    const UnorderedMap<String> & RedisClient::ServerInfo::operator()() const
    {
        return params;
    }

    RedisTransaction::RedisTransaction(RedisClient &client)
    : client(client) {}

    RedisClient::Response& RedisTransaction::exec()
    {
        // send MULTI command
        if (!inMulti) {
            iwarn("EXEC not support outside of MULTI transaction");
            return cachedResp;
        }

        // send EXEC command
        cachedResp = client.send("EXEC");
        if (!cachedResp) {
            // there is an error on the reply
            ierror("error sending 'EXEC' command: %s", cachedResp.error());
        }

        inMulti = false;
        return cachedResp;
    }

    bool RedisTransaction::discard()
    {
        if (!inMulti) {
            iwarn("DISCARD not supported outside MULTI");
            return false;
        }

        // send EXEC command
        cachedResp = client.send("DISCARD");
        if (!cachedResp) {
            // there is an error on the reply
            ierror("error sending 'DISCARD' command: %s", cachedResp.error());
        }

        inMulti = false;
        return cachedResp;
    }

    bool RedisTransaction::multi()
    {
        if (inMulti) {
            iwarn("MULTI not supported within a MULTI block");
            return false;
        }

        // send EXEC command
        cachedResp = client.send("MULTI");
        if (!cachedResp) {
            // there is an error on the reply
            ierror("error sending 'MULTI' command: %s", cachedResp.error());
        }

        inMulti = true;
        return cachedResp;
    }

    RedisTransaction::~RedisTransaction()
    {
        if (inMulti) {
            discard();
        }
    }

    RedisClient::RedisClient(net::Socket::UPtr&& sock, RedisDbConfig& config, CloseFunc onClose)
        : sock{std::move(sock)},
          config{config},
          onClose{std::move(onClose)}
    {}

    RedisClient::RedisClient()
        :config{redisDefaultConfig}
    {}

    RedisClient::RedisClient(RedisClient&& other)
        : sock{std::move(other.sock)},
          cacheId{std::move(other.cacheId)},
          config{other.config},
          onClose{std::move(other.onClose)},
          batched{std::move(other.batched)}
    {
        other.cacheId = CacheId{nullptr};
    }

    RedisClient& RedisClient::operator=(RedisClient&& other)
    {
        sock = std::move(other.sock);
        cacheId = other.cacheId;
        config = other.config;
        onClose = std::move(other.onClose);
        batched = std::move(other.batched);

        other.cacheId = CacheId{nullptr};
        other.onClose = nullptr;

        return Ego;
    }

    RedisClient::Response RedisClient::dosend(Command &cmd, size_t nrps)
    {
        // send the command to the server
        String data = cmd.prepared();
        size_t size = adaptor().send(data.data(), data.size(), config.Timeout);
        if (size != data.size()) {
            // sending failed somehow
            return Response{Reply('-', suil::catstr("sending failed: ", errno_s))};
        }
        adaptor().flush(config.Timeout);

        Response resp;
        std::vector<Reply> stage;
        do {
            if (!receiveResponse(resp.buffer, stage)) {
                // receiving data failed
                return Response{Reply('-',
                                      suil::catstr("receiving Response failed: ", errno_s))};
            }
        } while (--nrps > 0);

        resp.entries = std::move(stage);
        return std::move(resp);
    }

    String RedisClient::commit(Response& resp)
    {
        // send all the commands at once and read all the responses in one go
        auto last = batched.back();
        batched.pop_back();

        for (auto &cmd: batched) {
            String data = cmd->prepared();
            size_t size = adaptor().send(data.data(), data.size(), config.Timeout);
            if (size != data.size()) {
                // sending command failure
                return suil::catstr("sending '", (*cmd)(), "' failed: ", errno_s);
            }
        }

        resp = dosend(*last, batched.size() + 1);
        if (!resp) {
            // return error message
            return resp.error();
        }
        return String{nullptr};
    }

    bool RedisClient::receiveResponse(Buffer& out, std::vector<Reply>& staging)
    {
        std::size_t rxd{1}, offset{out.size()};
        char prefix;
        if (!adaptor().read(&prefix, rxd, config.Timeout)) {
            return false;
        }

        switch (prefix) {
            case PrefixError:
            case PrefixInteger:
            case PrefixValue: {
                if (!readLine(out)) {
                    return false;
                }
                String tmp{(char *) &out[offset], out.size() - offset, false};
                out << '\0';
                staging.emplace_back(Reply(prefix, std::move(tmp)));
                return true;
            }
            case PrefixString: {
                std::int64_t len{0};
                if (!readLen(len)) {
                    return false;
                }

                String tmp{};
                if (len >= 0) {
                    size_t size = (size_t) len + 2;
                    out.reserve((size_t) size + 2);
                    if (!adaptor().read(&out[offset], size, config.Timeout)) {
                        idebug("receiving string '%lu' failed: %s", errno_s);
                        return false;
                    }
                    // only interested in actual string
                    if (len) {
                        out.seek(len);
                        tmp = String{(char *) &out[offset], (size_t) len, false};
                        // terminate string
                        out << '\0';
                    }
                }
                staging.emplace_back(Reply(PrefixString, std::move(tmp)));
                return true;
            }

            case PrefixArray: {
                int64_t len{0};
                if (!readLen(len)) {
                    return false;
                }
                staging.emplace_back(Reply(PrefixArray, ""));
                if (len > 0) {
                    int i = 0;
                    for (i; i < len; i++) {
                        if (!receiveResponse(out, staging)) {
                            return false;
                        }
                    }
                }

                return true;
            }

            default: {
                ierror("received Response with unsupported type: %c", out[0]);
                return false;
            }
        }
    }

    bool RedisClient::readLen(int64_t &len)
    {
        Buffer out{16};
        if (!readLine(out)) {
            return false;
        }
        String tmp{(char *) &out[0], out.size(), false};
        tmp.data()[tmp.size()] = '\0';
        suil::cast(tmp, len);
        return true;
    }

    bool RedisClient::readLine(Buffer &out) {
        out.reserve(255);
        size_t cap{out.capacity()};
        do {
            size_t ndelims{1};
            if (!adaptor().receiveUntil(&out[out.size()], cap, "\r", ndelims, config.Timeout)) {
                if (errno == ENOBUFS) {
                    // reserve buffers
                    out.seek(cap);
                    out.reserve(512);
                    continue;
                }
                return false;
            }

            out.seek(cap - 1);
            uint8_t tmp[2];
            cap = sizeof(tmp);
            if (!adaptor().receiveUntil(tmp, cap, "\n", ndelims, config.Timeout)) {
                // this should be impossible
                return false;
            }
            break;
        } while (true);

        return true;
    }

    bool RedisClient::info(ServerInfo &out)
    {
        auto resp = send("INFO");

        if (!resp) {
            // couldn't receive Response
            ierror("failed to receive server information");
            out.Version = "NotFound";
            return false;
        }

        Reply &rp = resp.entries[0];
        auto parts = rp.data.parts("\r");
        for (auto &part: parts) {
            if (part.empty()) continue;
            String line = part.peek();
            if (line[0] == '\n') line = part.substr(1);
            if (line.empty() or line[0] == '#') continue;

            auto kv = line.parts(":");
            if (kv.size() != 2) {
                idebug("Unsupported key found " PRIs, _PRIs(line));
            }

            if (kv[0].compare("redis_version") == 0) {
                out.Version = kv[1].peek();
            } else {
                out.params.emplace(std::make_pair(kv[0].peek(), kv[1].peek()));
            }
        }

        out.buffer = std::move(resp.buffer);
        return true;
    }

    bool RedisClient::auth(const String& passwd)
    {
        return send("AUTH", passwd).status();
    }

    bool RedisClient::ping()
    {
        return send("PING").status("PONG");
    }

    size_t RedisClient::hlen(const String& hash)
    {
        auto resp = send("HLEN", hash);
        if (resp) {
            return (size_t) resp;
        }

        throw RedisDbError("redis HLEN '", hash, "' failed: ", resp.error());
    }

    std::int64_t RedisClient::incr(const String& key, int by)
    {
        Response resp;
        if (by == 0)
            resp = send("INCR", key);
        else
            resp = send("INCRBY", key, by);

        if (!resp) {
            throw RedisDbError("redis INCR '", key, "' failed: ", resp.error());
        }
        return (std::int64_t) resp;
    }

    int64_t RedisClient::decr(const String& key, int by)
    {
        Response resp;
        if (by == 0)
            resp = send("DECR", key);
        else
            resp = send("DECRBY", key, by);

        if (!resp) {
            throw RedisDbError("redis DECR '", key, "' failed: ", resp.error());
        }
        return (int64_t) resp;
    }

    String RedisClient::substr(const String& key, int start, int end)
    {
        auto resp = send("SUBSTR", key, start, end);
        if (resp) {
            return resp.get<String>(0);
        }
        throw RedisDbError("redis SUBSTR '", key,
                               "' start=", start, ", end=",
                               end, " failed: ", resp.error());
    }

    bool RedisClient::exists(const suil::String &key)
    {
        auto resp = send("EXISTS", key);
        if (resp) {
            return (int) resp != 0;
        }

        throw RedisDbError("redis EXISTS '", key, "' failed: ", resp.error());
    }

    bool RedisClient::del(const String& key)
    {
        auto resp = send("DEL", key);
        if (resp) {
            return (int) resp != 0;
        }

        throw RedisDbError("redis DEL '", key, "' failed: ", resp.error());
    }

    std::vector<String> RedisClient::keys(const String& pattern)
    {
        auto resp = send("KEYS", pattern);
        if (resp) {
            std::vector<String> tmp = resp;
            return std::move(tmp);
        }

        throw RedisDbError("redis KEYS pattern = '", pattern, "' failed: ", resp.error());
    }

    int RedisClient::expire(const String& key, int64_t secs)
    {
        auto resp =  send("EXPIRE", key, secs);
        if (resp) {
            return (int) resp;
        }
        throw RedisDbError("redis EXPIRE  '", key, "' secs ", secs,
                                    " failed: ", resp.error());
    }

    int RedisClient::ttl(const String& key)
    {
        auto resp =  send("TTL", key);
        if (resp) {
            return (int) resp;
        }
        throw RedisDbError("redis TTL  '", key, "' failed: ", resp.error());
    }

    int RedisClient::llen(const String& key)
    {
        auto resp = send("LLEN", key);
        if (resp) {
            return (int) resp;
        }
        throw RedisDbError("redis LLEN  '", key, "' failed: ", resp.error());
    }

    bool RedisClient::ltrim(const String& key, int start, int end)
    {
        auto resp = send("LTRIM", key, start, end);
        if (resp) {
            return resp.status();
        }

        throw RedisDbError("redis LTRIM[", start, ",", end, "'] ", key,
                                "' failed: ", resp.error());
    }

    int RedisClient::hdel(const String& hash, const String& key)
    {
        auto resp = send("HDEL", hash, key);
        if (resp) {
            return (int) resp;
        }
        throw RedisDbError("redis HDEL  '", hash, ":", key, "' failed: ", resp.error());
    }

    bool RedisClient::hexists(const String& hash, const String& key)
    {
        auto resp = send("HEXISTS", hash, key);
        if (resp) {
            return (int) resp != 0;
        }

        throw RedisDbError("redis HEXISTS  '", hash, ":", key, "' failed: ", resp.error());
    }

    std::vector<String> RedisClient::hkeys(const String& hash)
    {
        auto resp = send("HKEYS", hash);
        if (resp) {
            std::vector<String> keys = resp;
            return  std::move(keys);
        }
        throw RedisDbError("redis HKEYS  '", hash, "' failed: ", resp.error());
    }

    size_t RedisClient::scard(const String& set)
    {
        auto resp = send("SCARD", set);
        if (resp) {
            return  (size_t) resp;
        }
        throw RedisDbError("redis SCARD  '", set, "' failed: ", resp.error());
    }

    void RedisClient::reset()
    {
        batched.clear();
    }
    void RedisClient::batch(Command& cmd)
    {
        batched.push_back(&cmd);
    }

    void RedisClient::batch(std::vector<Command>& cmds)
    {
        for (auto& cmd: cmds) {
            batch(cmd);
        }
    }

    net::Socket& RedisClient::adaptor()
    {
        if (sock == nullptr) {
            throw RedisDbError("Connection to redis database is invalid");
        }
        return *sock;
    }

    void RedisClient::close()
    {
        if ((onClose != nullptr) and (cacheId != CacheId{nullptr})) {
            onClose(cacheId, true);
        }
    }

    RedisClient::~RedisClient() noexcept
    {
        if (onClose and (cacheId != CacheId{nullptr})) {
            onClose(cacheId, true);
            onClose = nullptr;
        }
        reset();
        if (sock != nullptr) {
            sock->close();
        }
    }

    RedisDb::RedisDb(const String& host, int port, RedisDbConfig config)
        : addr{ipremote(host(), port, 0, config.Timeout)},
          config{std::move(config)}
    {}

    RedisDb::~RedisDb() noexcept
    {
        aborting = true;
        if (cleaning) {
            pruneEvt.notify();
        }
    }

    const RedisClient::ServerInfo& RedisDb::getServerInfo()
    {
        if (!serverInfo.Version) {
            scoped(cli, connect(0));
            cli.info(serverInfo);
        }
        return serverInfo;
    }
    RedisClient& RedisDb::connect(int db)
    {
        auto it = fromCache(db);
        bool isFromCache{true};
        if (it == Clients::iterator{nullptr}) {
            it = newConnection();
            isFromCache = false;
        }

        auto& cli = *it;
        // ensure that the server is accepting commands
        if (!cli.ping()) {
            throw RedisDbError("redis - ping Request failed");
        }

        if (!isFromCache) {
            itrace("changing database to %d", db);
            auto resp = cli("SELECT", db);
            if (!resp) {
                throw RedisDbError(
                        "redis - changing to selected database '", db, "' failed: ", resp.error());
            }
        }

        return cli;
    }

    typename RedisDb::Clients::iterator RedisDb::fromCache(int db)
    {
        mill::Lock lk{cachedMutex};
        if (!Ego.cache.empty()) {
            auto handle = Ego.cache.front();
            lk.unlock();

            if (handle.db != db) {
                auto res = handle.it->send("SELECT", db);
                if (!res) {
                    idebug("Redis SELECT on cached connection error: %s", res.error());
                    handle.alive = mnow() - 500;
                    Ego.pruneEvt.notify();
                    // put back handle in cache, it will be cleaned up
                    lk.lock();
                    Ego.cache.push_front(handle);
                    return Clients::iterator{nullptr};
                }
            }

            return handle.it;
        }

        return Clients::iterator{nullptr};
    }

    typename RedisDb::Clients::iterator RedisDb::newConnection()
    {
        net::Socket::UPtr sock{nullptr};
        if (config.UseSsl) {
            sock = std::make_unique<net::SslSock>();
        }
        else {
            sock = std::make_unique<net::TcpSock>();
        }

        itrace("opening redis database connection");
        if (!sock->connect(addr, config.Timeout)) {
            throw RedisDbError("connecting to redis server '",
                               net::Socket::ipstr(addr), "' failed: ", errno_s);
        }

        RedisClient cli(std::move(sock), config, [&](typename Clients::iterator it, bool dctor) {
            Ego.returnConnection(it, dctor);
        });

        clientsMutex.acquire();
        auto it = Ego.clients.insert(Ego.clients.cend(), std::move(cli));
        it->cacheId = it;
        clientsMutex.release();

        if (config.Passwd) {
            if (it->auth(config.Passwd)) {
                throw RedisDbError("redis - authorizing client failed");
            }
        }

        itrace("connected to redis server: %s", net::Socket::ipstr(addr));
        return it;
    }

    void RedisDb::returnConnection(typename Clients::iterator it, bool dctor)
    {
        cache_handle_t entry{it, -1};
        if (!dctor && (config.KeepAlive != 0)) {
            {
                mill::Lock lk{cachedMutex};
                entry.alive = mnow() + Ego.config.KeepAlive;
                cache.push_back(entry);
            }
            bool expected{false};
            if (Ego.cleaning.compare_exchange_weak(expected, true)) {
                // schedule cleanup routine
                go(cleanup(Ego));
            }
        }
        else {
            mill::Lock lk{clientsMutex};
            if (it != clients.end()) {
                // caching not supported, delete client
                it->onClose = nullptr;
                clients.erase(it);
            }
        }
    }

    void RedisDb::cleanup(RedisDb& db)
    {
        int64_t expires = db.config.KeepAlive + 300;

        mill::Lock lk{db.cachedMutex};
        if (db.cache.empty()) {
            return;
        }

        do {
            db.pruneEvt.wait(lk, Deadline{expires});
            if (db.aborting)
                break;

            /* was not forced to exit */
            auto it = db.cache.begin();
            /* un-register all expired connections and all that will expire in the
             * next 500 ms */
            auto t = mnow() + 500;
            int pruned = 0;
            ltrace(&db, "starting prune with %ld connections", db.cache.size());
            while (it != db.cache.end()) {
                if ((*it).alive <= t) {
                    db.cache.erase(it);

                    lk.unlock();
                    {
                        mill::Lock cLk{db.clientsMutex};
                        if (db.isValid(it->it)) {
                            it->it->onClose = nullptr;
                            db.clients.erase(it->it);
                        }
                    }
                    lk.lock();

                    it = db.cache.begin();
                } else {
                    /* there is no point in going forward */
                    break;
                }

                if ((++pruned % 100) == 0) {
                    /* avoid hogging the CPU */
                    lk.unlock();
                    yield();
                    lk.lock();
                }
            }
            ltrace(&db, "pruned %ld connections", pruned);

            if (it != db.cache.end()) {
                /*ensure that this will run after at least 3 second*/
                expires = std::max((*it).alive - t, (int64_t)3000);
            }
        } while (!db.cache.empty() and !db.aborting);

        lk.unlock();

        if (db.aborting) {
            // remove all cached clients
            mill::Lock cLk{db.clientsMutex};
            db.clients.clear();
        }

        db.cleaning = false;
    }

    bool RedisDb::isValid(typename Clients::iterator& it)
    {
        return (it != Clients::iterator{nullptr}) and (it != clients.end());
    }
}