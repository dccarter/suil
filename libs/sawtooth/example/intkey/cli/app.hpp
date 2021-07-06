//
// Created by Mpho Mbotho on 2021-06-22.
//

#pragma once

#include <suil/base/args.hpp>
#include <suil/sawtooth/rest.hpp>
#include <suil/sawtooth/events.hpp>

namespace suil::saw {

    class App : public Client::RestApi {
    public:
        DISABLE_COPY(App);
        DISABLE_MOVE(App);

        App(args::Command& cmd);

        static void cmdSet(args::Command& cmd);
        static void cmdGet(args::Command& cmd);
        static void cmdIncrement(args::Command& cmd);
        static void cmdDecrement(args::Command& cmd);
        static void cmdList(args::Command& cmd);
        static void cmdBatch(args::Command& cmd);

    private:
        void postSubmit(const Return<String>& ret, uint64 wait);
        void modify(const String& verb);
        void get();
        void batch();
        void list();

        args::Command& mCmd;
    };
}
