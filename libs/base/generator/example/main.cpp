//
// Created by Mpho Mbotho on 2020-11-06.
//


#include "demo.scc.hpp"
#include <suil/base/wire.hpp>
#include <suil/base/json.hpp>

#include <iostream>
#include <variant>

int main(int argc, char* argv[])
{
    suil::StackBoard<2048> sb;
    suil::Command cmd(suil::Create("Carter", "Deposit"));
    get<suil::Create>(cmd.Value).Category = suil::Categories::Sports;

    sb << cmd;
    suil::Command cmd2;
    sb >> cmd2;
    std::visit([&](const auto& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, suil::Create>) {
            const suil::Create& create = cmd2;
            iod::foreach(suil::Create::Meta) | [&](const auto& m) {
                std::cout << m.symbol().name() << ": " << m.symbol().member_access(create) << std::endl;
            };
        }
    }, cmd2.Value);

    auto str = suil::json::encode(cmd2);
    std::cout << str << std::endl;
    suil::Command cmd3;
    suil::json::decode(str, cmd3);
    std::visit([&](const auto& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, suil::Create>) {
            iod::foreach(suil::Create::Meta) | [&](const auto& m) {
                std::cout << m.symbol().name() << ": " << m.symbol().member_access(arg) << std::endl;
            };
        }
    }, cmd3.Value);

    suil::JsonOnly jo;
    jo.name = "Carter";
    jo.obj = suil::json::Object(suil::json::Obj, "number", 4);
    str = suil::json::encode(jo);
    jo.other = jo.obj.weak();
    str = suil::json::encode(jo);

    suil::Categories category{suil::Categories::Sports};
    std::cout << category << std::endl;
    return 0;
}