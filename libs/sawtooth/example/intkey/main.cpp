//
// Created by Mpho Mbotho on 2021-06-16.
//

#include "tp.hpp"

#include <suil/base/args.hpp>
#include <suil/sawtooth/tp.hpp>

#define VALIDATOR_ENDPOINT "tcp://validator:4004"

using suil::Exception;
using suil::String;
using suil::args::Arguments;
using suil::args::Command;
using suil::args::Parser;
using suil::saw::TransactionProcessor;
using suil::saw::IntKeyHandler;

static void start(Command& cmd);

int main(int argc, char* argv[])
{
    Parser parser("int-key", "1.0", "Integer key sawtooth transaction processor");
    try {
        parser(
            Command("start", "Start running integer key transaction processor")
                ({"endpoint", "The address of the validator to connect to", 'C', false})
                (start)
                ()
        );
        parser.parse(argc, argv);
        parser.handle();
    }
    catch (...) {
        auto ex = Exception::fromCurrent();
        serror("unhandled error: %s", ex.what());
        return EXIT_FAILURE;
    }
}

void start(Command& cmd)
{
    constexpr const char * INTKEY_NAMESPACE = "int-key";
    auto config = cmd.value("endpoint", String{VALIDATOR_ENDPOINT}).dup();
    try {
        TransactionProcessor processor(std::move(config));
        auto& handler = processor.registerHandler<IntKeyHandler>(INTKEY_NAMESPACE, INTKEY_NAMESPACE);
        handler.getVersions().emplace_back("1.0");
        processor.run();
    }
    catch (...) {
        auto ex = Exception::fromCurrent();
        serror("%s", ex.what());
    }
}