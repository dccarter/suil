//
// Created by Mpho Mbotho on 2021-06-11.
//

#pragma once

#include <suil/base/args.hpp>
#include <suil/sawtooth/wallet.hpp>

namespace suil::saw::Stw {

    class StwLogger : public LogWriter {
    public:
        void write(const char *log, size_t size, Level level, const char *tag) override;
    };

    void initLogging();

    void cmdCreate(args::Command& cmd);
    void cmdGenerate(args::Command& cmd);
    void cmdGet(args::Command& cmd);
    void cmdList(args::Command& cmd);
}
