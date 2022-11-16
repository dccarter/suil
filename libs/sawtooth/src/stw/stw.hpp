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

    void cmdCreate(suil::Command& cmd);
    void cmdGenerate(suil::Command& cmd);
    void cmdGet(suil::Command& cmd);
    void cmdList(suil::Command& cmd);
}
