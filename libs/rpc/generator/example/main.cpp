//
// Created by Mpho Mbotho on 2020-11-30.
//

#include "suil/rpc/generator/kvstore.scc.hpp"
#include "suil/rpc/json/server.hpp"
#include "suil/rpc/srpc/server.hpp"
#include "kvstore.hpp"
#include <suil/base/logging.hpp>
#include <scc/clipp.hpp>
#include <iostream>

namespace rpc = suil::rpc;
namespace jrpc = rpc::jrpc;
namespace srpc = rpc::srpc;

template <typename S>
static coroutine void startServer(S& s)
{
    s.start();
}

using namespace clipp;

void cmdServer(int port)
{
    auto store = std::make_shared<rpc::KVStore>();
    auto jctx = std::make_shared<rpc::KVStoreJrpcContext>(store);
    auto sctx = std::make_shared<rpc::KVStoreSrpcContext>(store);
    sctx->init();
    port = port == 0? 5000: port;

    suil::net::TcpSocketConfig srpcConfig{
        .bindAddr = {
                .port = port
        }
    };

    suil::net::TcpSocketConfig jrpcConfig{
        .bindAddr = {
                .port = port+1
        }
    };

    jrpc::Server server(jctx, opt(tcpConfig, jrpcConfig));
    server.listen();
    srpc::Server  srpcServer(sctx, opt(tcpConfig, srpcConfig));
    srpcServer.listen();

    go(startServer(srpcServer));
    startServer(server);
}


template <typename C>
static void cmdSet(C& client, const suil::String& key, const suil::String& value)
{
    if (!client.connect()) {
        serror("connecting to server failed....");
        exit(EXIT_FAILURE);
    }
    try {
        client.set(key, value);
    }
    catch (...) {
        auto ex = suil::Exception::fromCurrent();
        serror("%s", ex.what());
    }
}

void cmdSet(bool json, int port, const suil::String& key, const suil::String& value)
{
    port = (port == 0)? (json? 5001: 5000) : port;
    suil::net::TcpSocketConfig rpcConfig{
        .bindAddr = {
            .port = port
        }
    };
    if (json) {
        rpc::KVStoreJrpcClient client(opt(tcpConfig, rpcConfig));
        cmdSet(client, key, value);
    }
    else {
        rpc::KVStoreSrpcClient client(opt(tcpConfig, rpcConfig));
        cmdSet(client, key, value);
    }
}

template <typename C>
static void cmdGet(C& client, const suil::String& key)
{
    suil::setup(opt(verbose, suil::Level::CRITICAL));
    if (!client.connect()) {
        serror("connecting to server failed....");
        exit(EXIT_FAILURE);
    }
    try {
        auto res = client.get(key);
        if (res) {
            printf(PRIs " = " PRIs "\n", _PRIs(key), _PRIs(res));
        }
        else {
            printf("key " PRIs " not found\n", _PRIs(key));
        }
    }
    catch (...) {
        auto ex = suil::Exception::fromCurrent();
        serror("%s", ex.what());
    }
}

void cmdGet(bool json, int port, const suil::String& key)
{
    port = (port == 0)? (json? 5001: 5000) : port;
    suil::net::TcpSocketConfig rpcConfig{
            .bindAddr = {
                    .port = port
            }
    };
    if (json) {
        rpc::KVStoreJrpcClient client(opt(tcpConfig, rpcConfig));
        cmdGet(client, key);
    }
    else {
        rpc::KVStoreSrpcClient client(opt(tcpConfig, rpcConfig));
        cmdGet(client, key);
    }
}

int main(int argc, char *argv[])
{
    enum class mode {help, server, get, set, list};
    mode selected{mode::help};
    std::string helpCmd{};
    bool jsonClient{false};
    int  startPort{0};
    std::string key{}, value{};

    auto helpMode = (
        command("help").set(selected, mode::help),
        opt_value("cmd", helpCmd)
    );

    auto serverMode = (
        command("server").set(selected, mode::server),
        (option("-P", "--port") & opt_value("port", startPort)) % "Optional port that servers will listen on"
    );

    auto getMode = (
        command("get").set(selected, mode::get),
        (option("-j", "--json").set(jsonClient)) % "Optional parameter to select json client",
        (option("-P", "--port") & opt_value("port", startPort)) % "The port the server is listening on",
        (required("-K", "--key") & clipp::value("key", key)) % "The key to retrieve"
    );

    auto setMode = (
            command("set").set(selected, mode::set),
            (option("-j", "--json").set(jsonClient)) % "Optional parameter to select json client",
            (required("-K", "--key")   & clipp::value("key", key)) % "The key to set",
            (required("-V", "--value") & clipp::value("value", value)) % "The value to set to",
            (option("-P", "--port")  & opt_value("port", startPort)) % "The port the server is listening on"
    );

    auto cli = (
            (helpMode | serverMode | getMode | setMode));

    if (parse(argc, argv, cli)) {
        switch (selected) {
            case mode::help:
                break;
            case mode::server:
                cmdServer(startPort);
                break;
            case mode::set:
                cmdSet(jsonClient, startPort, suil::String{key}, suil::String{value});
                break;
            case mode::get:
                cmdGet(jsonClient, startPort, suil::String{key});
                break;
            default:
                serror("unsupported command");
        }
    }
    else {
        std::cerr << usage_lines(cli, "scc") << '\n';
    }
    return 0;
}