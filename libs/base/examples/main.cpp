//
// Created by Mpho Mbotho on 2020-12-30.
//

#include "suil/base/notify.hpp"
#include "suil/base/args.hpp"

using suil::String;
using suil::fs::Watcher;
using suil::fs::Event;
using suil::fs::Events;

AddCommand(dassem, "disassembles the given bytecode file instead of running it",
    Positionals(Str(Name("file"), Help("Path to the file containing the bytecode to disassemble"))),
    Opt(Name("hide-addr"), Sf('H'), Help("Hide instruction addresses from generated assembly")),
    Str(Name("output"), Sf('o'),
        Help("Path to the output file, if not specified the disassembly will be dumped to console"), Def(""))
)
{
    return EXIT_SUCCESS;
}

AddCommand(run, "runs the given bytecode file, parsing any command line arguments "
             "following the '--' marker to the program.",
    Positionals(Str(Name("path"), Help("Path to the file containing the bytecode to run"))),
    Bytes(Name("Xss"), Help("Adjust the virtual machine stack size"), Def("8K")),
    Bytes(Name("Xms"),
          Help("Adjust the total memory to allocate for the virtual machine. This "
               "value should be larger that the stack size as the stack is chunked "
               "from the total allocated memory."),
          Def("1M"))
)
{
    printf("run(%s, Xss=%u, Xms=%u\n",
        cmd.at<const char*>(0), cmd.get<uint32>("Xss"), cmd.get<uint32>("Xms"));
    return EXIT_SUCCESS;
}

void cmdDassem(suil::Command *cmd, int argc, char **argv);

int main(int argc, char *argv[])
{
    SuilParser("demo", "0.0.1",
           Commands(AddCmd(run), AddCmd(dassem)),
           DefaultCmd(run));


    return suil::cmdExecute(parser, argc, argv);
}