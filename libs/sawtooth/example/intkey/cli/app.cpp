//
// Created by Mpho Mbotho on 2021-06-22.
//

#include "app.hpp"
#include <suil/sawtooth/transaction.hpp>

namespace suil::saw {

    App::App(args::Command& cmd)
        : Client::RestApi(
                cmd.value("url", "http://rest-api"_str),
                "int-key",
                "1.0",
                cmd.value("key", "2c5683d43573214fa939d2df4ac0767fe3500b6eacbafa11cf8b8aeca8db9d60"_str),
                cmd.value<int32_t>("port", 8008)
                ),
           mCmd{cmd}
    {}

    void App::modify(const String& verb)
    {
        auto name = mCmd.at<String>(0,
                                    "Missing key name - usage: intkey-cli set|dec|inc [...] name value");
        auto value = mCmd.at<int32>(1,
                                    "Missing key value - usage: intkey-cli set|dec|inc [...] name value");
        auto wait  = Ego.mCmd.value<uint64>("wait", 0) * 1000;

        json::Object obj(json::Obj,
                 "Verb", verb,
                 "Name", name,
                 "Value", value);
        auto str = json::encode(obj);
        auto prefix = Ego.getPrefix();

        auto ret = Ego.asyncBatches(fromStdString(str), {prefix}, {prefix});
        postSubmit(ret, wait);

    }

    void App::get()
    {
        auto name = mCmd.at<String>(0, "Missing key name - usage: intkey-cli get [...] endpoint name");
        auto ret = Ego.getState(name);
        if (!ret) {
            serror("failed to retrieve key state - %s", ret.exception().what());
            return;
        }

        auto state = ret.moveValue();
        if (!state.empty()) {
            auto obj = json::Object::decode(state);
            auto value = (int32_t) obj(name());
            sinfo(PRIs " = %d", _PRIs(name), value);
        }
    }

    void App::list()
    {
        auto ret = Ego.getStates(Ego.getPrefix());
        if (!ret) {
            serror("failed to retrieve keys - %s", ret.exception().what());
            return;
        }

        auto states = ret.moveValue();
        for (auto& state: states) {
            auto obj = json::Object::decode(state);
            for (auto& [key, val] : obj) {
                auto num = (int) val;
                sinfo("%s = %d", key, num);
            }
        }
    }

    void App::batch()
    {
        auto count = Ego.mCmd.value<uint32>("count", 10);
        auto wait  = Ego.mCmd.value<uint64>("wait", 0) * 1000;
        auto& Enc = Ego.encoder();
        auto prefix = Ego.getPrefix();
        std::vector<Client::Transaction> txns;
        for (int i = 0; i < count; i++) {
            json::Object jobj(json::Obj,
                              "Verb",  "set",
                              "Name",  suil::catstr("Key",i),
                              "Value", 100+i);
            auto str = json::encode(jobj);
            txns.push_back(Enc(fromStdString(str), {prefix}, {prefix}));
        }

        auto batch = Enc(txns);
        auto ret = Ego.asyncBatches({batch});
        postSubmit(ret, wait);
    }

    void App::postSubmit(const Return<String>& ret, uint64 wait)
    {
        if (!ret) {
            serror("failed to submit batch - %s", ret.exception().message().c_str());
            return;
        }

        if (wait == 0) {
            sinfo("Create batch integer keys submitted {batch_id = " PRIs "}", _PRIs(ret.value()));
            return;
        }

        if (!Ego.waitForStatus(ret.value(), wait, [&](auto& s) { return s != "PENDING"; } )) {
            serror("Timeout waiting for batch to be committed {batch_id = " PRIs "}", _PRIs(ret.value()));
        }
    }

    void App::cmdSet(args::Command& cmd)
    {
        App app(cmd);
        app.modify("set");
    }

    void App::cmdGet(args::Command& cmd)
    {
        App app(cmd);
        app.get();
    }

    void App::cmdBatch(args::Command& cmd)
    {
        App app(cmd);
        app.batch();
    }

    void App::cmdDecrement(args::Command& cmd)
    {
        App app(cmd);
        app.modify("dec");
    }

    void App::cmdIncrement(args::Command& cmd)
    {
        App app(cmd);
        app.modify("inc");
    }

    void App::cmdList(args::Command& cmd)
    {
        App app(cmd);
        app.list();
    }
}
