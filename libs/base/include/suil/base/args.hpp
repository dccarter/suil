//
// Created by Mpho Mbotho on 2021-06-05.
//

#pragma once

#include <suil/base/string.hpp>
#include <suil/base/exception.hpp>

namespace suil::args {

    constexpr const char NOSF{'\0'};

    struct Arg {
        String Lf{};
        String Help{};
        char   Sf{NOSF};
        bool   Option{true};
        bool   Required{false};
        bool   Global{false};
        String Prompt{};
        bool   Hidden{false};
        String Default{};

    private:
        friend struct Command;

        inline bool check(const String& arg) const {
            return arg == Lf;
        }

        inline bool check(char c) const { return c == Sf; }

        inline bool check(char c, const String& l) {
            return (c != NOSF && check(c)) || check(l);
        }
    };

    struct Arguments {
        template <typename... A>
        Arguments(A... argv)
                : Argv{new char*[sizeof...(A)]},
                  Argc{sizeof...(A)}
        {
            add(argv...);
        }

        Arguments(const char** argv, int argc)
            : Argv{new char*[argc]},
              Argc{argc}
        {
            for (int i = 0; i < argc; i++) {
                Argv[i] = strdup(argv[i]);
            }
        }

        Arguments(const Arguments&) = delete;
        Arguments& operator=(const Arguments&) = delete;
        Arguments(Arguments&&) = delete;
        Arguments& operator=(Arguments&&) = delete;

        ~Arguments() {
            if (Argv != nullptr) {
                for (int i = 0; i < Argc; i++) {
                    free(Argv[i]);
                }
                delete[] Argv;
                Argv = nullptr;
            }
        }
        char **Argv{nullptr};
        int Argc{0};

    private:
        template <typename... A>
        void add(const char *a, A... argv) {
            Argv[Argc - sizeof...(A) - 1] = strdup(a);
            if constexpr(sizeof...(A) > 0) {
                add(argv...);
            }
        }
    };

    struct Command {
        using Handler = std::function<void(Command&)>;

        Command(String name, String desc = nullptr, bool help = false);

        Command& operator()(Arg&& arg);
        Command& operator()(Handler handler);
        Command  operator()() { return std::move(Ego); }
        void showHelp(String app, Buffer& help, bool isHelp = false) const;
        bool parse(int argc, char *argv[], bool debug = false);
        bool parse(Arguments& arguments, bool debug = false) {
            return parse(arguments.Argc, arguments.Argv, debug);
        }

        String at(int index, const String& errMsg = nullptr);

        String operator[](char sf)
        {
            auto& arg = Ego.check(nullptr, sf);
            return Ego[arg.Lf].peek();
        }

        String operator[](const String& lf);

        String operator[](int index) {
            return Ego.at(index);
        }

        template <typename V>
        V at(int index, const String& errMsg = nullptr)
        {
            String value = Ego.at(index, errMsg);
            V tmp{};
            setValue(tmp, value);
            return tmp;
        }

        template <typename V>
        V value(const String& lf, V def)
        {
            Arg* _;
            if (!check(_, lf, NOSF)) {
                throw InvalidArguments("passed parameter '", lf, "' is not an argument");
            }

            auto zStr = Ego[lf];
            if constexpr(std::is_same_v<V, String>) {
                if (!zStr.empty()) {
                    return std::move(zStr);
                }
                else {
                    return std::move(def);
                }
            }
            else {
                V tmp{std::move(def)};
                if (!zStr.empty()) {
                    setValue(tmp, zStr);
                }
                return std::move(tmp);
            }
        }

    private suil_ut:
        friend class Parser;
        inline void setValue(String& out, String& from) {
            out = std::move(from);
        }

        template <typename V>
        inline void setValue(V& out, String& from) {
            suil::cast(from.trim(' '), out);
        }

        template <typename V>
        inline void setValue(std::vector<V>& out, String& from) {
            auto parts = from.split(",");
            for (auto& part: parts) {
                V val{};
                setValue(val, part);
                out.push_back(std::move(val));
            }
        }

        void requestValue(Arg& arg);
        Arg& check(const String& lf, char sf);
        bool check(Arg*& found, const String& lf, char sf);

        static int isValid(const String&  flag);

        String mName{};
        String mDesc{};
        std::vector<Arg> mArgs{};
        std::vector<String> mPositionals{};
        size_t mLongest{0};
        bool mRequired{false};
        bool mInternal{false};
        bool mInteractive{false};
        bool mHasGlobalFlags{false};
        bool mTakesArgs{false};
        Handler mHandler{nullptr};
        UnorderedMap<String, CaseInsensitive> mParsed{};
    };

    class Parser {
    public:
        Parser(String app, String version, String descript = nullptr);
        template <typename... T>
        Parser& operator()(T... obj) {
            Ego.add(std::forward<T>(obj)...);
            return Ego;
        }

        void  parse(int argc, char *argv[]);
        void  parse(Arguments& arguments) { parse(arguments.Argc, arguments.Argv); }
        void  handle();
        void  getCommandHelp(Buffer& out, Command& cmd, bool isHelp);
        const Command* getCommand() const {
            return mParsed;
        }

        template <typename T>
        String operator[](char sf) {
            String _{nullptr};
            Arg *arg = findArg(_, sf);
            if (arg) {
                return Ego.getValue(arg->Lf, arg);
            }
            return String{};
        }

        String operator[](const String& lf)
        {
            return getValue(lf, nullptr);
        }

    private suil_ut:
        template <typename... Commands>
        Parser& add(Command&& cmd, Commands&&... cmds)
        {
            add(std::move(cmd));
            if constexpr(sizeof...(cmds) > 0) {
                add(std::forward<Commands>(cmds)...);
            }
            return Ego;
        }

        template <typename... Args>
        Parser& add(Arg&& arg, Args&&... as)
        {
            add(std::move(arg));
            if constexpr(sizeof...(Args) > 0) {
                add(std::forward<Args>(as)...);
            }
            return Ego;
        }

        Parser& add(Command&& cmd);
        Parser& add(Arg&& arg);
        template <typename... T>
        Parser& add(std::function<void(Parser&)> fn) {
            fn(Ego);
            return Ego;
        }

        String    getValue(const String&, Arg* arg);
        void      showHelp(const char *prefix = nullptr);
        String    getHelp(const char *prefix = nullptr);
        Command*  find(const String& name);
        Arg* findArg(const String& name, char sf=NOSF);
        Arg  shallowCopy(const Arg& arg);
        std::vector<Command>  mCommands;
        std::vector<Arg> mGlobals;
        // this is the command that successfully passed
        Command   *mParsed{nullptr};
        String    mAppName;
        String    mDescription;
        String    mAppVersion;
        size_t    mLongestCmd{0};
        size_t    mLongestFlag{0};
        bool      mTakesArgs{false};
        bool      mInter{false};
    };

    Status<String> readParam(const String& display, const String& def);
    Status<String> readPasswd(const String& display);
}