//
// Created by Mpho Mbotho on 2021-06-11.
//

#define SUIL_CMD_PREFIX_NAME
#include "stw.hpp"

namespace suil::saw::Stw {

    void StwLogger::write(const char *log, size_t size, Level level, const char *tag)
    {
        static Syslog sSysLog{"suil-stw"};
        switch (level) {
            case Level::INFO:
            case Level::NOTICE:
                printf("%*s", (int)size, log);
                break;
            case Level::ERROR:
            case Level::CRITICAL:
            case Level::WARNING:
            case Level::DEBUG:
            case Level::TRACE:
                sSysLog.write(log, size, level, nullptr);
                break;
            default:
                break;
        }
    }

    void initLogging()
    {
        suil::setup(
            opt(writer, std::make_shared<StwLogger>()),
            opt(format, [&](char *out, Level l, const char *tag, const char *fmt, va_list args) {
                switch (l) {
                    case Level::DEBUG:
                    case Level::TRACE:
                    case Level::ERROR:
                    case Level::CRITICAL:
                    case Level::WARNING:
                        return DefaultFormatter()(out, l, tag, fmt, args);
                    default: {
                        int wr = vsnprintf(out, SUIL_LOG_BUFFER_SIZE, fmt, args);
                        out[wr++] = '\n';
                        out[wr]   = '\0';
                        return (size_t) wr;
                    }
                }
            }));
    }

    void cmdCreate(suil::Command& cmd)
    {
        auto path = cmd.get<const char*>("path");
        auto passwd = cmd.get<const char*>("passwd");

        auto wall = Client::Wallet::create(path, passwd);
        wall.save();
    }

    void cmdGenerate(suil::Command& cmd)
    {
        auto path = cmd.get<const char*>("path");
        auto passwd = cmd.get<const char*>("passwd");
        auto name = cmd.get<const char*>("name");

        auto wall = Client::Wallet::open(path, passwd);
        wall.generate(name);
        wall.save();
    }

    void cmdGet(suil::Command& cmd)
    {
        auto path = cmd.get<const char*>("path");
        auto passwd = cmd.get<const char*>("passwd");
        auto name =cmd.get<const char*>("name");

        auto wall = Client::Wallet::open(path, passwd);
        const auto& key = wall.get(name);
        if (key.empty()) {
            serror("Key %s does not exist", name);
        }
        else {
            sinfo(PRIs, _PRIs(key));
        }
    }

    void cmdList(suil::Command& cmd)
    {
        auto path = cmd.get<const char*>("path");
        auto passwd = cmd.get<const char*>("passwd");
        auto wall = Client::Wallet::open(path, passwd);
        const auto& schema = wall.schema();
        for (const auto& key: schema.Keys) {
            sinfo(PRIs, _PRIs(key.Name));
        }
    }
}