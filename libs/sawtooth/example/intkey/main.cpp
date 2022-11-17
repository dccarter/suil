//
// Created by Mpho Mbotho on 2021-06-16.
//

#include "tp.hpp"

#include <suil/base/args.hpp>
#include <suil/sawtooth/tp.hpp>

#define VALIDATOR_ENDPOINT "tcp://validator:4004"

using suil::Exception;
using suil::String;
using suil::saw::TransactionProcessor;
using suil::saw::IntKeyHandler;

AddCommand(start, "Start running integer key transaction processor",
    Positionals(),
    Str(Name("endpoint"),
        Sf('C'),
        Help("The address of the validator to connect to"),
        Def(VALIDATOR_ENDPOINT))
)
{
    constexpr const char * INTKEY_NAMESPACE = "int-key";
    auto config = cmd.get<const char*>("endpoint");
    try {
        TransactionProcessor processor(String{config}.dup());
        auto& handler = processor.registerHandler<IntKeyHandler>(INTKEY_NAMESPACE, INTKEY_NAMESPACE);
        handler.getVersions().emplace_back("1.0");
        processor.run();

        return EXIT_SUCCESS;
    }
    catch (...) {
        auto ex = Exception::fromCurrent();
        serror("%s", ex.what());

        return EXIT_FAILURE;
    }
}

int main(int argc, char* argv[])
{
    SuilParser("int-key", "1.0",
        Commands(AddCmd(start)),
        DefaultCmd(start));
    
    return suil::cmdExecute(parser, argc, argv);
}
