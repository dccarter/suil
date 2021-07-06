//
// Created by Mpho Mbotho on 2021-06-11.
//

#include <suil/base/args.hpp>
#include "stw.hpp"

using suil::DefaultFormatter;
using suil::Level;
using suil::LogWriter;
using suil::Syslog;

using suil::args::Arg;
using suil::args::Arguments;
using suil::args::Command;
using suil::args::Parser;

namespace Stw = suil::saw::Stw;

int main(int argc, const char* argv[])
{
    Stw::initLogging();
    Parser parser("stw", "0.1.0", "An application used to create/edit a wallet file");

    parser(
        Arg{.Lf = "passwd", .Help = "The secret used to lock/unlock the wallet file",
            .Option = false, .Required = true, .Prompt = "Enter password: ", .Hidden = true},
        Arg{.Lf = "path", .Help = "A path to save the created wallet file (default: .sawtooth.key)", .Option = false},
        Command("create", "Create a new wallet file initialized with a master key")
          (Stw::cmdCreate)
          (),
        Command("gen", "Generate and add a new key to the given wallet file")
          ({.Lf = "name", .Help = "The name of the key to generate", .Option = false, .Required = true})
          (Stw::cmdGenerate)
          (),
        Command("get", "Get a key saved on the given wallet file")
          ({.Lf = "name", .Help = "The name of the key to retrieve", .Option = false})
          (Stw::cmdGet)
          (),
        Command("list", "List all the keys saved in the current wallet")
          (Stw::cmdList)
          ()
    );

    try {
        Arguments args{argv, argc};
        parser.parse(args);
        parser.handle();
    }
    catch (...) {
        fprintf(stderr, "%s\n", suil::Exception::fromCurrent().what());
        return EXIT_FAILURE;
    }
}