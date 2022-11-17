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

        App(suil::Command& cmd);

        static void cmdSet(suil::Command& cmd);
        static void cmdGet(suil::Command& cmd);
        static void cmdIncrement(suil::Command& cmd);
        static void cmdDecrement(suil::Command& cmd);
        static void cmdList(suil::Command& cmd);
        static void cmdBatch(suil::Command& cmd);

    private:
        void postSubmit(const Return<String>& ret, uint64 wait);
        void modify(const String& verb);
        void get();
        void batch();
        void list();

        suil::Command& mCmd;
    };
}
