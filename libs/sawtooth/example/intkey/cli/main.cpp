//
// Created by Mpho Mbotho on 2021-06-22.
//

#include "app.hpp"


using suil::args::Arguments;
using suil::args::Command;
using suil::args::Arg;
using suil::args::Parser;
using suil::Exception;

using suil::saw::App;

int main(int argc, char *argv[])
{
    Parser parser("intkey-cli", "1.0", "Application used to interact with int-key transaction processor");
    try {
        parser(
            Arg{"url", "The address of sawtooth rest api endpoint", 'H', false},
            Arg{"port", "The port on which sawtooth rest api endpoint is listening", 'P', false},
            Arg{"key", "The private key used to sign the transactions", 'K', false},
            Command("set", "Create a new integer key with the given name and value")
                ({"wait", "The number of seconds to wait for batch to be committed (default: 0)", 'W', false})
                (App::cmdSet)
                (),
            Command("inc", "Increment the given key with the given value")
                ({"wait", "The number of seconds to wait for batch to be committed (default: 0)", 'W', false})
                (App::cmdIncrement)
                (),
            Command("dec", "Decrement the given key with the given value")
                ({"wait", "The number of seconds to wait for batch to be committed (default: 0)", 'W', false})
                (App::cmdDecrement)
                (),
            Command("get", "Get the value associated with the given keys")
                (App::cmdGet)
                (),
            Command("list", "List all key created for int-key transaction processor")
                (App::cmdList)
                (),
            Command("batch", "Generate and set integer keys on int-key as a batch")
                ({"count", "The number of integer keys to generate (default: 10)", 'C', false})
                ({"wait", "The number of seconds to wait for batch to be committed (default: 0)", 'W', false})
                (App::cmdBatch)
                ()
        );

        parser.parse(argc, argv);
        parser.handle();

        return EXIT_SUCCESS;
    }
    catch(...) {
        serror("unhandled error %s", Exception::fromCurrent().what());
        return EXIT_FAILURE;
    }
}