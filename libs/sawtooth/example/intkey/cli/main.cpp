//
// Created by Mpho Mbotho on 2021-06-22.
//

#include "app.hpp"

using suil::Exception;

using suil::saw::App;

AddCommand(set, "Create a new integer key with the given name and value",
    Positionals(
        Str(Name("name"),
            Help("The name of the key to set")),
        Int(Name("Value"),
            Help("The value to be assigned to the key"))
    ),
    Int(Name("wait"),
        Sf('W'),
        Help("The number of seconds to wait for batch to be committed (default: 0)"),
        Def("0"))
)
{
    App::cmdSet(cmd);
    return 0;
}

AddCommand(inc, "Increment the given key with the given value",
    Positionals(
        Str(Name("name"),
            Help("The name of the key to increment"))
    ),
    Int(Name("wait"),
        Sf('W'),
        Help("The number of seconds to wait for batch to be committed (default: 0)"),
        Def("0"))
)
{
    App::cmdIncrement(cmd);
    return 0;
}

AddCommand(dec, "Decrement the given key with the given value",
    Positionals(
        Str(Name("name"),
            Help("The name of the key to decrement"))
    ),
    Int(Name("wait"),
        Sf('W'),
        Help("The number of seconds to wait for batch to be committed (default: 0)"),
        Def("0"))
)
{
    App::cmdDecrement(cmd);
    return 0;
}

AddCommand(get, "Get the value associated with the given keys",
    Positionals(
        Str(Name("name"),
            Help("The name of the key to retrieve"))
    ))
{
    App::cmdGet(cmd);
    return 0;
}

AddCommand(list, "List all key created for int-key transaction processor", Positionals())
{
    App::cmdList(cmd);
    return 0;
}

AddCommand(batch, "Generate and set integer keys on int-key as a batch",
    Positionals(),
    Int(Name("count"), 
        Sf('C'),
        Help("The number of integer keys to generate (default: 10)"), 
        Def("0")),
    Int(Name("wait"),
        Sf('W'),
        Help("The number of seconds to wait for batch to be committed (default: 0)"),
        Def("0"))
)
{
    App::cmdBatch(cmd);
    return 0;
}

int main(int argc, char *argv[])
{
    SuilParser("intkey-cli", "1.0",
        Commands(AddCmd(set), AddCmd(inc), AddCmd(dec),
                 AddCmd(get), AddCmd(list), AddCmd(batch)),
        DefaultCmd(list),
        Str(Name("url"),
            Sf('H'),
            Help("The address of sawtooth rest api endpoint"),
            Def("http://rest-api")),
        Int(Name("port"),
            Sf('P'),
            Help("The port on which sawtooth rest api endpoint is listening"),
            Def("8008")),
        Str(Name("key"),
            Sf('K'),
            Help("The private key used to sign the transactions"),
            Def("2c5683d43573214fa939d2df4ac0767fe3500b6eacbafa11cf8b8aeca8db9d60"))
    );

    return suil::cmdExecute(parser, argc, argv);
}