//
// Created by Mpho Mbotho on 2020-11-19.
//

#ifndef SUILDB_REDIS_HPP
#define SUILDB_REDIS_HPP

#include <suil/db/config.scc.hpp>

#include <deque>
#include <list>
#include <suil/base/channel.hpp>
#include <suil/base/blob.hpp>
#include <suil/base/exception.hpp>
#include <suil/net/socket.hpp>

namespace suil::db {

    define_log_tag(REDIS_DB);
    DECLARE_EXCEPTION(RedisDbError);

    class RedisClient final : LOGGER(REDIS_DB) {
        static constexpr const char* REDIS_CRLF                 = "\r\n";
        static constexpr const char* StatusOk            = "OK";
        static constexpr char PrefixError         = '-';
        static constexpr char PrefixValue         = '+';
        static constexpr char PrefixString        = '$';
        static constexpr char PrefixArray         = '*';
        static constexpr char PrefixInteger       = ':';

        using CacheId = typename std::list<RedisClient>::iterator;
        using CloseFunc = std::function<void(CacheId, bool)>;

        struct Command final {
            template <typename ...Params>
            Command(const String& cmd, const Params&... params)
                : buffer{32+cmd.size()}
            {
                prepare(cmd, params...);
            }

            String operator()() const;

            template<typename Param>
            Command& operator<<(const Param& param) {
                addparam(param);
                return Ego;
            }

            template<typename ...Params>
            inline void params(const Params&... params) {
                addparams(params...);
            }

            inline String prepared() const {
                return String{buffer.data(), buffer.size(), false};
            }

        private:
            template <typename... Params>
            void prepare(const String& cmd, const Params&... params) {
                buffer << PrefixArray << (1 + sizeof...(Params)) << REDIS_CRLF;
                addparams(cmd, params...);
            }

            void addparam(const String& param);

            template <typename T>
                requires std::is_arithmetic<T>::value
            inline void addparam(const T& param) {
                addparam(suil::tostr(param));
            }

            inline void addparam(const Data& param) {
                addHexParam(param.data(), param.size());
            }

            template <size_t B>
            inline void addparam(const Blob<B>& param) {
                addHexParam(&param.cbin(), param.size());
            }

            template <typename T, typename... Params>
            void addparams(const T& param, const Params&... params) {
                addparam(param);
                if constexpr (sizeof...(params) > 0) {
                    addparams(params...);
                }
            }

            void addHexParam(const void *data, size_t size);

            Buffer buffer;
        };

        struct Reply {
            Reply(char prefix, String data);

            Reply(Buffer rxb);

            operator bool() const;

            bool status(const char *expect = StatusOk) const;

            const char* error() const;

            bool special() const;

            String data{nullptr};
            Buffer received{};
            char   prefix{'-'};
        };

    public:
        struct Response {
            Response() = default;
            Response(Reply rp);

            operator bool() const;

            const char* error() const;

            bool status(const char *expect = StatusOk) const;

            template <typename T>
            const T get(int index = 0) const {
                if (index >= entries.size()) {
                    throw RedisDbError("index '", index, "' out of range '", entries.size(), "'");
                }

                T d{};
                castreply(index, d);
                return std::move(d);
            }

            template<typename T>
                requires std::is_arithmetic_v<T>
            operator std::vector<T>() const {
                std::vector<T> tmp;
                int i{0};
                if (not(entries.empty()) and entries.front().prefix == PrefixArray) {
                    i = 1;
                }

                for (; i < entries.size(); i++) {
                    tmp.push_back(get<T>(i));
                }

                return std::move(tmp);
            }

            operator std::vector<String>() const {
                std::vector<String> tmp;
                int i = 0;
                if (not(entries.empty()) && entries.front().prefix == PrefixArray)
                    i = 1;
                for (i; i < entries.size(); i++) {
                    tmp.emplace_back(get<String>(i));
                }
                return  std::move(tmp);
            }

            template <typename T>
            operator T() const {
                return  std::move(get<T>(0));
            }

            String peek(int index = 0) const;

            void operator|(std::function<bool(const String&)> func);

            std::vector<Reply> entries;
            Buffer             buffer{128};

        private:
            template<typename T>
                requires std::is_arithmetic_v<T>
            void castreply(int index, T& d) const {
                suil::cast(entries[index].data, d);
            }
            void castreply(int index, String& d) const;
        };

        struct ServerInfo {
            ServerInfo() = default;
            DISABLE_COPY(ServerInfo);
            MOVE_CTOR(ServerInfo) = default;
            MOVE_ASSIGN(ServerInfo) = default;

            const String& operator[](const String& key) const;
            const UnorderedMap<String>& operator()() const;
            String Version;
        private:
            friend RedisClient;
            UnorderedMap<String> params;
            Buffer buffer;
        };

    public:

        RedisClient();
        RedisClient(net::Socket::UPtr&& sock, RedisDbConfig& config, CloseFunc onClose);

        DISABLE_COPY(RedisClient);
        MOVE_CTOR(RedisClient);
        MOVE_ASSIGN(RedisClient);

        inline Response send(Command& cmd) {
            return dosend(cmd, 1);
        }

        template <typename ...Args>
        Response send(const String& cmd, Args... args) {
            Command command(cmd, std::forward<Args>(args)...);
            return send(command);
        }

        template <typename... Args>
        inline Response operator()(const String& cd, Args... args) {
            return send(cd, std::forward<Args>(args)...);
        }

        inline void flush() {
            reset();
        }

        bool auth(const String& passwd);

        bool ping();

        template <typename T>
        auto get(const String& key) -> T {
            auto resp = send("GET", key);
            if (!resp) {
                throw RedisDbError("redis GET '", key, "' failed: ", resp.error());
            }
            return static_cast<T>(resp);
        }

        template <typename T>
        bool set(const String& key, const T& val) {
            return send("SET", key, val).status();
        }

        std::int64_t incr(const String& key, int by = 0);
        std::int64_t decr(const String& key, int by = 0);

        template <typename T>
        bool append(const String& key, const T& val) {
            return send("APPEND", key, val).status();
        }

        String substr(const String& key, int start, int end);

        bool exists(const String& key);

        bool del(const String& key);

        std::vector<String> keys(const String& pattern);

        int expire(const String& key, std::int64_t secs);

        int ttl(const String& key);

        template <typename... T>
        int rpush(const String& key, T... vals) {
            auto resp = send("RPUSH", key, std::forward<T>(vals)...);
            if (resp) {
                return (int) resp;
            }
            else {
                throw RedisDbError("redis RPUSH  '", key, "' failed: ", resp.error());
            }
        }

        template <typename... T>
        int lpush(const String& key, T... vals) {
            Response resp = send("LPUSH", key, std::forward<T>(vals)...);
            if (resp) {
                return (int) resp;
            }
            else {
                throw RedisDbError("redis LPUSH  '", key, "' failed: ", resp.error());
            }
        }

        int llen(const String& key);

        template <typename T>
        auto lrange(const String& key, int start = 0, int end = -1) -> std::vector<T> {
            auto resp = send("LRANGE", key, start, end);
            if (resp) {
                return  std::vector<T>(resp);
            }
            else {
                throw RedisDbError("redis LRANGE  '", key, "' failed: ", resp.error());
            }
        }

        bool ltrim(const String& key, int start = 0, int end = -1);

        template <typename T>
        auto ltrim(const String& key, int index) -> T {
            auto resp = send("LINDEX", key, index);
            if (resp) {
                return (T) resp;
            }
            else {
                throw RedisDbError("redis LINDEX  '", key, "' failed: ", resp.error());
            }
        }

        int hdel(const String& hash, const String& key);

        bool hexists(const String& hash, const String& key);

        std::vector<String> hkeys(const String& hash);

        template <typename T>
        auto hvals(const String& hash) -> std::vector<T> {
            auto resp = send("HVALS", hash);
            if (resp) {
                return  (std::vector<T>) resp;
            }
            else {
                throw RedisDbError("redis HVALS  '", hash, "' failed: ", resp.error());
            }
        }

        template <typename T>
        auto hget(const String& hash, const String& key) -> T {
            auto resp = send("HGET", hash, key);
            if (!resp) {
                throw RedisDbError("redis HGET '", hash, ":", key, "' failed: ", resp.error());
            }
            return (T) resp;
        }

        template <typename T>
        inline bool hset(const String& hash, const String& key, const T val) {
            auto resp = send("HSET", hash, key, val);
            if (!resp) {
                throw RedisDbError("redis HSET '", hash, ":", key, "' failed: ", resp.error());
            }
            return (int) resp != 0;
        }

        size_t hlen(const String& hash);

        template <typename... T>
        int sadd(const String& set, const T... vals) {
            auto resp = send("SADD", set, vals...);
            if (resp) {
                return (int) resp;
            }
            else {
                throw RedisDbError("redis SADD  '", set, "' failed: ", resp.error());
            }
        }

        template <typename T>
        auto smembers(const String& set) -> std::vector<T>{
            auto resp = send("SMEMBERS", set);
            if (resp) {
                return  (std::vector<T>) resp;
            }
            else {
                throw RedisDbError("redis SMEMBERS  '", set, "' failed: ", resp.error());
            }
        }

        template <typename T>
        auto spop(const String& set) -> T {
            Response resp = send("spop", set);
            if (!resp) {
                throw RedisDbError("redis spop '", set, "' failed: ", resp.error());
            }
            return (T) resp;
        }


        size_t scard(const String& set);


        bool info(ServerInfo&);

        void close();

        ~RedisClient();

    private:
        Response dosend(Command& cmd, size_t nreply);
        void reset();
        void batch(Command& cmd);
        void batch(std::vector<Command>& cmds);
        bool receiveResponse(Buffer& out, std::vector<Reply>& staging);
        bool readLine(Buffer& out);
        bool readLen(std::int64_t& len);
        String commit(Response& resp);
        net::Socket& adaptor();

    private:
        friend class RedisDb;
        net::Socket::UPtr sock{nullptr};
        CacheId      cacheId{nullptr};
        RedisDbConfig& config;
        CloseFunc onClose{nullptr};
        std::vector<Command*> batched;
    };

    struct RedisTransaction : LOGGER(REDIS_DB) {
        template <typename... Params>
        bool operator()(const char *cmd, Params... params) {
            if (Ego.inMulti) {
                auto resp = client.send(cmd, params...);
                return resp.status();
            }
            iwarn("Executing command '%s' in invalid transaction", cmd);
            return false;
        }

        RedisClient::Response& exec();

        bool discard();

        bool multi();

        RedisTransaction(RedisClient& client);

        virtual ~RedisTransaction();

    private:
        RedisClient::Response cachedResp;
        RedisClient&         client;
        bool                 inMulti{false};
    };

    class RedisDb final : LOGGER(REDIS_DB) {
        using Clients = std::list<RedisClient>;
        struct cache_handle_t final {
            typename Clients::iterator it;
            int     db{0};
            int64_t alive{0};
        };
        using ClientsCache = std::deque<cache_handle_t>;

    public:
        RedisDb(const String& host, int port, RedisDbConfig config);
        RedisDb() = default;

        template <typename ...Args>
        void setup(const String& host, int port, Args... args) {
            addr = ipremote(host(), port, 0, Deadline{3000});
            suil::applyConfig(Ego.config, std::forward<Args>(args)...);
        }

        template <typename Config, typename ...Configs>
            requires iod::has_symbol<RedisDbConfig::Schema, typename Config::left_t>::value
        RedisDb& configure(const Config& cfg, const Configs&... others) {
            cfg.left.member_access(config) = cfg.right;
            if constexpr (sizeof...(others) > 0) {
                return setup(std::forward<Configs>(others)...);
            }
            else {
                return Ego;
            }
        }

        RedisClient& connect(int db = 0);

        const RedisClient::ServerInfo& getServerInfo();

    private:
        typename Clients::iterator fromCache(int db);
        typename Clients::iterator newConnection();
        void returnConnection(typename Clients::iterator it, bool dctor);
        static coroutine void cleanup(RedisDb& db);
        bool isValid(typename Clients::iterator& it);
    private:
        Clients  clients;
        ClientsCache cache;
        ipaddr  addr;
        RedisDbConfig config;
        RedisClient::ServerInfo serverInfo;
        Channel<uint8_t>  notify{1};
        bool           cleaning{false};
    };
}

#endif //SUILDB_REDIS_HPP
