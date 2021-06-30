//
// Created by Mpho Mbotho on 2021-06-05.
//

#include "suil/base/args.hpp"

#include <suil/base/exception.hpp>
#include <suil/base/buffer.hpp>
#include <suil/base/console.hpp>

namespace suil::args {

    Command::Command(String name, String desc, bool help)
        : mName{std::move(name)},
          mDesc{std::move(desc)}
    {
        if (help) {
            Ego(Arg{"help", "Show this command's help", 'h', true, false});
        }
    }

    Command& Command::operator()(Handler handler)
    {
        Ego.mHandler = std::move(handler);
        return Ego;
    }

    Command& Command::operator()(Arg&& arg)
    {
        if (!arg.Lf) {
            throw InvalidArguments("Command line argument missing long format (--arg) option");
        }

        for (auto& a: mArgs) {
            if (a.check(arg.Sf, arg.Lf)) {
                throw InvalidArguments("Command line '", a.Lf, "|", arg.Sf, "' duplicated");
            }
        }

        mRequired = mRequired || arg.Required;
        mTakesArgs = mTakesArgs || !arg.Option;
        auto len = arg.Option? arg.Lf.size(): arg.Lf.size() + 5;
        mLongest = std::max(mLongest, len);
        arg.Global = false;
        mArgs.emplace_back(std::move(arg));
        return Ego;
    }

    void Command::showHelp(String app, Buffer& help, bool isHelp) const
    {
        if (isHelp) {
            help << mDesc << "\n\n";
        }
        help << "Usage:\n"
             << "  " << app << " " << mName;
        if (!mArgs.empty()) {
            help << " [flags...]\n"
                 << "\n"
                 << "Flags:\n";

            for (auto& arg: mArgs) {
                if (arg.Global) {
                    continue;
                }

                size_t remaining{mLongest+1};
                help << "    ";
                if (arg.Sf != NOSF) {
                    help << '-' << arg.Sf;
                    help << ", ";
                } else {
                    help << "    ";
                }

                help << "--" << arg.Lf;
                help << (arg.Prompt? '?': (arg.Required? '*': ' '));
                help << (arg.Option? (mTakesArgs? "     ": ""): " arg0");

                remaining -= arg.Lf.size();
                help << String{' ', remaining} << arg.Help
                     << "\n";
            }
        }
        else {
            help << "\n";
        }
    }

    Arg& Command::check(const String& lf, char sf)
    {
        Arg *arg{nullptr};
        if (check(arg, lf, sf)) {
            return *arg;
        }
        else {
            if (lf) {
                throw InvalidArguments("error: command argument '--", lf, "' is not recognized");
            }
            else {
                throw InvalidArguments("error: command argument '-", sf, "' is not recognized");
            }
        }
    }

    bool Command::check(Arg*& found, const String& lf, char sf)
    {
        for (auto& arg: mArgs) {
            if (arg.check(sf, lf)) {
                found = &arg;
                return true;
            }
        }
        return false;
    }

    bool Command::parse(int argc, char** argv, bool debug)
    {
        int pos{0};
        String zArg{};
        bool isHelp{false};

        while (pos < argc) {
            auto cArg = argv[pos];
            auto cVal = strchr(cArg, '=');
            if (cVal != nullptr) {
                // argument is --arg=val
                *cVal = '\0';
            }
            if (cArg[0] != '-') {
                // positional argument
                Ego.mPositionals.push_back(String{cArg}.dup());
                pos++;
                continue;
            }

            int dashes = isValid(cArg);
            if (dashes == 0) {
                throw InvalidArguments("error: Unsupported argument syntax '", cArg, "'");
            }

            if (dashes == 2) {
                // argument is parsed in Lf
                auto& arg = Ego.check(&cArg[2], NOSF);
                if (arg.Prompt) {
                    throw InvalidArguments("error: command argument '", arg.Lf, "' is supported through prompt");
                }

                if (arg.Sf == 'h') {
                    isHelp = true;
                    break;
                }

                if (mParsed.find(arg.Lf) != mParsed.end()) {
                    throw InvalidArguments("error: command argument '", arg.Lf, "' was already passed");
                }
                String val{"1"};
                if (!arg.Option) {
                    if (cVal == nullptr) {
                        pos++;
                        if (pos >= argc) {
                            throw InvalidArguments("error: command argument '", arg.Lf, "' expects a value");
                        }
                        cVal = argv[pos];
                    }
                    val = String{cVal}.dup();
                }
                else if (cVal != nullptr) {
                    throw InvalidArguments("error: command argument '", arg.Lf, "' does not take a value");
                }
                mParsed.emplace(arg.Lf.peek(), std::move(val));
            }
            else {
                size_t nOpts = strlen(cArg);
                size_t oPos{1};
                while (oPos < nOpts) {
                    auto& arg = Ego.check(nullptr, cArg[oPos++]);
                    if (arg.Sf == 'h') {
                        isHelp = true;
                        break;
                    }

                    String val{"1"};
                    if (!arg.Option) {
                        if (oPos < nOpts) {
                            throw InvalidArguments("error: command argument '", arg.Lf, "' expects a value");
                        }

                        if (cVal == nullptr) {
                            pos++;
                            if (pos >= argc) {
                                throw InvalidArguments("error: command argument '", arg.Lf, "' expects a value");
                            }
                            cVal = argv[pos];
                        }

                        val = String{cVal}.dup();
                    }
                    else if (cVal != nullptr) {
                        throw InvalidArguments("error: command argument '", arg.Lf,
                                               "' assigned a value but it's an option");
                    }
                    mParsed.emplace(arg.Lf.peek(), std::move(val));
                }

                if (isHelp) {
                    break;
                }
            }
            pos++;
        }

        if (!isHelp) {
            Buffer msg{127};
            msg << "error: missing required arguments:";
            bool missing{false};
            for (auto& arg: mArgs) {
                if (!arg.Required or (mParsed.find(arg.Lf) != mParsed.end())) {
                    continue;
                }
                if (arg.Prompt) {
                    // Command argument is interactive, request value from console
                    requestValue(arg);
                }
                else {
                    msg << (missing? ", '" : " '") << arg.Lf << '\'';
                    missing = true;
                }
            }

            if (missing) {
                throw InvalidArguments(String{msg});
            }
        }

        if (debug) {
            // dump all arguments to console
            for (auto& kvp: mParsed) {
                printf("--%s = %s\n", kvp.first(), kvp.second.c_str("nil"));
            }
        }

        return isHelp;
    }

    void Command::requestValue(Arg& arg)
    {
        Status<String> status;
        if (arg.Hidden) {
            status = readPasswd(arg.Prompt);
        }
        else {
            status = readParam(arg.Prompt, arg.Default);
        }
        if (status.error and arg.Required) {
            throw InvalidArguments("error: required interactive argument '", arg.Lf, "' not provided");
        }

        mParsed.emplace(arg.Lf.peek(), std::move(status.result));
    }

    String Command::at(int index, const String& errMsg)
    {
        if (mPositionals.size() <= index) {
            if (errMsg) {
                throw InvalidArguments(errMsg);
            }
            return {};
        }
        return mPositionals[index].peek();
    }

    String Command::operator[](const String& lf)
    {
        auto it = mParsed.find(lf);
        if (it != mParsed.end()) {
            return it->second.peek();
        }
        return {};
    }

    int Command::isValid(const String& flag)
    {
        if (flag[0] != '-' || (flag.size() == 1)) {
            return 0;
        }

        if (flag[1] != '-') {
            return (flag[1] != '\0' and ::isalpha(flag[1]))? 1: 0;
        }

        if (flag.size() == 2) {
            return 0;
        }

        return (flag[2] != '\0' and isalpha(flag[2]))? 2: 0;
    }

    Parser::Parser(String app, String version, String descript)
        : mAppName{std::move(app)},
          mAppVersion{std::move(version)},
          mDescription{std::move(descript)}
    {
        // Add a global flag
        Ego(Arg{"help", "Show help for the active command for application", 'h'});

        // add version command
        Command ver{"version", "Show the application version"};
        ver([&](Command& cmd) {
            Buffer ob{63};
            ob << mAppName << " v" << mAppVersion << "\n";
            if (!Ego.mDescription.empty()) {
                ob << Ego.mDescription << "\n";
            }
            write(STDOUT_FILENO, ob.data(), ob.size());
        });
        ver.mInternal = true;

        // add help command
        Command help{"help", "Display this application help"};
        help([&](Command& cmd) {
            // show application help
            Ego.showHelp();
        });
        help.mInternal = true;

        Ego.add(std::move(ver), std::move(help));
    }

    Command* Parser::find(const String &name)
    {
        Command *cmd{nullptr};
        for (auto& c: mCommands) {
            if (c.mName == name) {
                cmd = &c;
                break;
            }
        }

        return cmd;
    }

    Arg* Parser::findArg(const String &name, char sf)
    {
        for (auto& a: mGlobals) {
            if (a.Lf == name || (sf != NOSF and sf == a.Sf)) {
                return &a;
            }
        }
        return nullptr;
    }

    Arg Parser::shallowCopy(const Arg &arg)
    {
        return std::move(Arg{arg.Lf.peek(), arg.Help.peek(), arg.Sf,
                             arg.Option, arg.Required, true,
                             arg.Prompt.peek(), arg.Hidden, arg.Default.peek()});
    }

    Parser& Parser::add(Arg &&arg)
    {
        if (Ego.findArg(arg.Lf) == nullptr) {
            arg.Global = true;
            size_t len = (arg.Option? arg.Lf.size() : arg.Lf.size() + 5);
            mTakesArgs = mTakesArgs || !arg.Option;
            mLongestFlag = std::max(mLongestFlag, len);
            mGlobals.emplace_back(std::move(arg));
        }
        else {
            throw InvalidArguments(
                    "duplicate global argument '", arg.Lf, "' already registered");
        }
        return Ego;
    }

    Parser& Parser::add(Command &&cmd)
    {
        if (Ego.find(cmd.mName) == nullptr) {
            // add copies of global arguments
            for (auto& ga: mGlobals) {
                Arg* _;
                if (!cmd.check(_, ga.Lf, ga.Sf)) {
                    Arg copy = Ego.shallowCopy(ga);
                    cmd.mHasGlobalFlags = true;
                    cmd.mArgs.emplace_back(std::move(copy));
                }
            }

            // accommodate interactive
            mInter = mInter || cmd.mInteractive;
            size_t len = Ego.mInter? (cmd.mName.size()+14) : cmd.mName.size();
            mLongestCmd = std::max(mLongestCmd, len);
            // add a new command
            mCommands.emplace_back(std::move(cmd));
        }
        else {
            throw InvalidArguments(
                    "command with name '", cmd.mName, " already registered");
        }
        return Ego;
    }

    String Parser::getHelp(const char *prefix)
    {
        Buffer out{254};
        if (prefix != nullptr) {
            out << prefix << '\n';
        }
        else {
            if (mDescription) {
                // show description
                out << mDescription << '\n';
            }
            else {
                // make up description
                out << mAppName;
                if (mAppVersion) {
                    out << " v" << mAppVersion;
                }
                out << '\n';
            }
        }
        out << '\n';

        out << "Usage:"
            << "    " << mAppName << " [command ...]\n"
            << '\n';

        // append commands help
        if (!mCommands.empty()) {
            out << "Available Commands:\n";
            for (auto& cmd: mCommands) {
                size_t remaining{mLongestCmd - cmd.mName.size()};
                out << "  " << cmd.mName << ' ';
                if (cmd.mInteractive) {
                    out << "(interactive) ";
                    remaining -= 14;
                }
                if (remaining > 0) {
                    std::string str;
                    str.resize(remaining, ' ');
                    out << str;
                }
                out << cmd.mDesc << '\n';
            }
        }

        // append global arguments
        if (!mGlobals.empty()) {
            out << "Flags:\n";
            for (auto& ga: mGlobals) {
                size_t remaining{mLongestFlag - ga.Lf.size()};
                out << "    ";
                if (ga.Sf != NOSF) {
                    out << '-' << ga.Sf;
                    if (ga.Lf)
                        out << ", ";
                } else {
                    out << "    ";
                }

                out << "--" << ga.Lf;
                out << (ga.Prompt? '?': (ga.Required? '*': ' '));
                out << (ga.Option? (mTakesArgs? "     ": ""): " arg0");

                std::string str;
                str.resize(remaining, ' ');
                out << str << " " << ga.Help;
                out << "\n";
            }
        }
        out << '\n'
            << "Use \"" << mAppName
            << "\" [command] --help for more information about a command"
            << "\n\n";
        return String{out};
    }

    void Parser::showHelp(const char *prefix) {

        String str{Ego.getHelp(prefix)};
        write(STDOUT_FILENO, str.data(), str.size());
    }

    void Parser::getCommandHelp(Buffer& out, Command& cmd, bool isHelp)
    {
        // help was requested or an error occurred
        if (!out.empty()) {
            out << "\n";
        }

        cmd.showHelp(mAppName, out, isHelp);
        // append global arguments
        if (cmd.mHasGlobalFlags) {
            out << "\nGlobal Flags:\n";
            for (auto& ga: cmd.mArgs) {
                if (!ga.Global) {
                    continue;
                }

                size_t remaining{mLongestFlag - ga.Lf.size()};
                out << "    ";
                if (ga.Sf != NOSF) {
                    out << '-' << ga.Sf;
                    if (ga.Lf)
                        out << ", ";
                } else {
                    out << "    ";
                }

                out << "--" <<  ga.Lf;
                out << (ga.Prompt? '?': (ga.Required? '*': ' '));
                out << (ga.Option? (mTakesArgs? "     ": ""): " arg0");

                std::string str;
                str.resize(remaining, ' ');
                out << str << " " << ga.Help;
                out << "\n";
            }
        }
        out << "\n";
    }

    void Parser::parse(int argc, char **argv)
    {
        if (argc <= 1) {
            // show application help
            Ego.showHelp();
            exit(EXIT_FAILURE);
        }

        if (argv[1][0] == '-') {
            int tr = Command::isValid(argv[1]);
            if (!tr) {
                fprintf(stderr, "error: bad flag syntax: %s\n", argv[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(stderr, "error: free floating flags are not supported\n");
            exit(EXIT_FAILURE);
        }

        String  cmdstr{argv[1]};
        Command *cmd = Ego.find(cmdstr);
        if (cmd == nullptr) {
            fprintf(stderr, "error: unknown command \"%s\" for \"%s\"\n",
                    argv[1], mAppName());
            exit(EXIT_FAILURE);
        }

        bool showhelp[2] ={false, true};
        Buffer errbuf{126};
        try {
            // parse command line (appname command)
            int nargs{argc-2};
            char  **args = nargs? &argv[2] : &argv[1];
            showhelp[0] = cmd->parse(argc-2, args);
        }
        catch (Exception& ser) {
            showhelp[0] = true;
            showhelp[1] = false;
            errbuf << ser.what() << "\n";
        }

        if (showhelp[0]) {
            //
            Ego.getCommandHelp(errbuf, *cmd, showhelp[1]);
            write(STDOUT_FILENO, errbuf.data(), errbuf.size());
            exit(showhelp[1]? EXIT_SUCCESS:EXIT_FAILURE);
        }

        // save passed command
        mParsed = cmd;

        if (cmd->mInternal) {
            // execute internal commands and exit
            Ego.handle();
            exit(EXIT_SUCCESS);
        }
    }

    void Parser::handle() {
        if (mParsed && mParsed->mHandler) {
            mParsed->mHandler(*mParsed);
            return;
        }
        throw UnsupportedOperation("parser::parse should be "
                                "invoked before invoking handle");
    }

    Status<String> readParam(const String& display, const String& def)
    {
        write(STDIN_FILENO, display.data(), display.size());
        sync();
        auto [err, val] = Console::in().readLine();
        if (err == 0) {
            return Ok(std::move(val));
        }
        else if (def) {
            return Ok(def.dup());
        }

        return {-1};
    }

    Status<String> readPasswd(const String& display)
    {
        auto pass = getpass(display());
        return Ok(String{pass}.dup());
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>
#define ARGV_SIZE(argv) sizeof(argv)/sizeof(const char *)

using suil::args::Arg;
using suil::args::Command;
using suil::args::Parser;
using suil::args::Arguments;

SCENARIO("Using args::Parser")
{
    WHEN("Building a command") {
        Command cmd("one", "Simple one command", false);
        REQUIRE(cmd.mName == "one");
        REQUIRE(cmd.mDesc == "Simple one command");
        cmd({.Lf = "help", .Help = "Show shelp", .Sf = 'h'});
        REQUIRE(cmd.mArgs.size() == 1);
    }

    WHEN("Showing command help") {
        Command cmd("one", "Simple one command", false);
        cmd({.Lf = "help", .Help = "Show shelp", .Sf = 'h'})
           ({.Lf = "one", .Help = "First argument", .Sf = 'o', .Option = false})
           ({.Lf = "two", .Help = "Second argument", .Required = true})
           ({.Lf = "three", .Help = "Third argument", .Required = true, .Global = true})
           ({.Lf =  "four", .Help = "Fourth argument", .Required = true, .Prompt = "Enter 4:"})
           ;

        suil::Buffer help{1024};
        cmd.showHelp("demo", help, false);
        suil::String expected =
                "Usage:\n"
                "  demo one [flags...]\n"
                "\n"
                "Flags:\n"
                "    -h, --help           Show shelp\n"
                "    -o, --one  arg0      First argument\n"
                "        --two*           Second argument\n"
                "        --three*         Third argument\n"
                "        --four?          Fourth argument\n"
                ;
        REQUIRE(suil::String{help} == expected);
    }

    WHEN("Using Command::parse") {
        Command cmd("doit", "Simple one command", false);
        cmd({.Lf = "help", .Help = "Show shelp", .Sf = 'h'})
                ({.Lf = "one", .Help = "First argument", .Sf = 'o', .Option = false})
                ({.Lf = "two", .Help = "Second argument", .Required = true})
                ({.Lf = "three", .Help = "Third argument", .Sf = 't', .Required = true, .Global = true});

        {
            Arguments args{"--help"};
            REQUIRE(cmd.parse(args.Argc, args.Argv, false));
            cmd.mParsed.clear();
        }

        {
            Arguments args{"--one", "one", "--two", "-t"};
            REQUIRE_FALSE(cmd.parse(args, false));
            cmd.mParsed.clear();
        }

        {
            Arguments args{"-o", "1", "--two", "-t"};
            REQUIRE_FALSE(cmd.parse(args, false));
            cmd.mParsed.clear();
        }

        {
            Arguments args{"--one", "--two", "-t"};
            REQUIRE_THROWS(cmd.parse(args, false));
            cmd.mParsed.clear();
        }

        {
            Arguments args{"--one", "1" "--two"}; // missing required option
            REQUIRE_THROWS(cmd.parse(args, false));
            cmd.mParsed.clear();
        }

        {
            Arguments args{"--", "1" "--two"}; // invalid syntax
            REQUIRE_THROWS(cmd.parse(args, false));
            cmd.mParsed.clear();
        }
    }

    WHEN("Building a Parser") {
        Parser parser("demo", "0.1.0", "Test application");
        parser(Command{"add", "Add two numbers", false});
        {
            REQUIRE_THROWS(parser(Command{"add", "Add two numbers"})); // Duplicate command names not allowed
        }

        parser(Arg{"verbose", "Show all commands output", 'v'});
        {
            REQUIRE_THROWS(parser(Arg{"verbose", "Show all commands output", 'v'}));
        }
    }

    WHEN("Showing parser help") {
        Parser parser("demo", "0.1.0", "Test application");
        parser(
            Arg{"verbose", "Show verbose output", 'v', true},
            Command("add", "Add some numbers", false)
              ({.Lf ="start", .Help = "first addend", .Sf = 'a', .Required = true})
              ({.Lf = "next", .Help = "second addend", .Sf = 'b', .Required = true})(),
            Command("sub", "Subtract some numbers from the minuend", false)
             ({"minuend", "The number to subtract from", 'm'})()
        );

        {
            auto help = parser.getHelp();
            suil::String expected = "Test application\n"
                                    "\n"
                                    "Usage:    demo [command ...]\n"
                                    "\n"
                                    "Available Commands:\n"
                                    "  version Show the application version\n"
                                    "  help    Display this application help\n"
                                    "  add     Add some numbers\n"
                                    "  sub     Subtract some numbers from the minuend\n"
                                    "Flags:\n"
                                    "    -h, --help     Show help for the active command for application\n"
                                    "    -v, --verbose  Show verbose output\n"
                                    "\n"
                                    "Use \"demo\" [command] --help for more information about a command\n"
                                    "\n";
            REQUIRE(help == expected);
        }
        {
            auto& cmd = parser.mCommands[2];
            suil::Buffer out{64};
            parser.getCommandHelp(out, cmd, false);
            suil::String help{out};
            suil::String expected = "Usage:\n"
                                    "  demo add [flags...]\n"
                                    "\n"
                                    "Flags:\n"
                                    "    -a, --start* first addend\n"
                                    "    -b, --next*  second addend\n"
                                    "\n"
                                    "Global Flags:\n"
                                    "    -h, --help     Show help for the active command for application\n"
                                    "    -v, --verbose  Show verbose output\n"
                                    "\n";
            REQUIRE(help == expected);
        }
    }

    WHEN("Parsing command line arguments") {
        Parser parser("demo", "0.1.0", "Test application");
        parser(
                Command("add", "Add some numbers", false)
                        ({"start", "first addend", 'a', false, true})
                        ({"next", "second addend", 'b', false, true})(),
                Command("sub", "Subtract some numbers from the minuend", false)
                        ({"minuend", "The number to subtract from", 'm', false, true})(),
                Arg{"verbose", "Show verbose output", 'v', true}
        );

        {
            Arguments arguments{"demo", "add", "--start", "100", "--next", "10", "13"};
            REQUIRE_NOTHROW(parser.parse(arguments));
            REQUIRE(parser.mParsed != nullptr);

            auto& cmd = *parser.mParsed;
            REQUIRE(cmd.value("start", int(0)) == 100);
            REQUIRE(cmd.value("next", int(0)) == 10);
            REQUIRE(cmd.at<int>(0) == 13);
        }
    }
}

#endif