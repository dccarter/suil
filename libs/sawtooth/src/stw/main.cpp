//
// Created by Mpho Mbotho on 2021-06-11.
//
#define SUIL_CMD_PREFIX_NAME
#include <suil/base/args.hpp>
#include "stw.hpp"

using suil::DefaultFormatter;
using suil::Level;
using suil::LogWriter;
using suil::Syslog;

namespace Stw = suil::saw::Stw;

AddCommand(create, "Create a new wallet file initialized with a master key",
  Positionals()
)
{
    Stw::cmdCreate(cmd);
    return EXIT_SUCCESS;
}

AddCommand(gen, "Generate and add a new key to the given wallet file",
  Positionals(),
  Str(ArgName("name"), Help("The name of the key to generate"))
)
{
    Stw::cmdGenerate(cmd);
    return EXIT_SUCCESS;
}

AddCommand(get, "Get a key saved on the given wallet file",
  Positionals(),
  Str(ArgName("name"), Help("The name of the key to retrieve"))
)
{
    Stw::cmdGet(cmd);
    return EXIT_SUCCESS;
}

AddCommand(list, "List all the keys saved in the current wallet",
  Positionals()
)
{
    Stw::cmdList(cmd);
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
    Stw::initLogging();
    SuilParser("stw", "0.1.0",
        Commands(AddCmd(create), AddCmd(gen), AddCmd(get), AddCmd(list)),
        DefaultCmd(list),
        PromptPassword(
            ArgName("passwd"),
            Help("The secret used to lock/unlock the wallet file")),
        Str(ArgName("path"),
            Help("A path to save the created wallet file"),
            Def(".sawtooth.key")));
    try {
        return suil::cmdExecute(parser, argc, argv);
    }
    catch(...) {
      auto ex = suil::Exception::fromCurrent();
      fprintf(stderr, "error: %s\n", ex.what());
      return EXIT_FAILURE;
    }
}