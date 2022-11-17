/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-21
 */

#pragma once

#include <suil/base/logging.hpp>
#include <stdio.h>
#include <optional>
#include <variant>

namespace suil {
    constexpr char NO_SF{'\0'};

    using  CmdFlagValue = std::variant<std::monostate, bool, double, const char*>;
    struct Command;

    struct CmdFlag {
        const char *name{nullptr};
        char sf{NO_SF};
        const char *help{nullptr};
        const char *def{nullptr};
        const void *promptCtx{nullptr};
        bool(*prompt)(Command *, CmdFlag *, const void*){nullptr};
        CmdFlagValue val{};
        bool  isAppOnly{false};
        bool(*validator)(struct CmdParser*, CmdFlagValue*, const char *, const char *){nullptr};
    };

    struct CmdPositional {
        const char *name{nullptr};
        const char *help{nullptr};
        const char *def{nullptr};
        CmdFlagValue val{};
        bool(*validator)(CmdParser*, CmdFlagValue*, const char *, const char*);
    };

    struct Command {
        const char *name{nullptr};
        const char *help{nullptr};
        int(*handler)(Command&, int, char**);
        uint32         nargs{0};
        uint32         npos{0};
        CmdPositional *pos{nullptr};
        struct CmdParser *P{nullptr};
        uint16         lp{0};
        uint16         la{0};
        int32          id{0};
        CmdFlag       *args{nullptr};

        std::optional<CmdFlagValue> getFlag(const char *name);
        std::optional<CmdFlagValue> getFlag(uint32 i);
        std::optional<CmdFlagValue> getGlobalFlag(uint32 i);
        std::optional<CmdFlagValue> getPositional(uint32 i);

        template <typename T>
            requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
        auto get(uint32 i, bool global = false) -> T {
            if (global) {
                auto value = getGlobalFlag(i);
                suilAssert0(value && std::holds_alternative<double>(*value));
                return (T) std::get<double>(*value);
            }
            else {
                auto value = getFlag(i);
                suilAssert0(value && std::holds_alternative<double>(*value));
                return (T) std::get<double>(*value);
            }
        }

        template <typename T>
            requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
        auto get(const char *name) -> T {
            auto value = getFlag(name);
            suilAssert(value && std::holds_alternative<double>(*value), "%s", name);
            return (T) std::get<double>(*value);
        }

        template <typename T>
            requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
        auto at(uint32 i) -> T {
            auto value = getPositional(i);
            suilAssert0(value && std::holds_alternative<double>(*value));
            return (T) std::get<double>(*value);
        }

        template <typename T>
            requires (std::is_enum_v<T>)
        auto get(uint32 i, bool global = false) -> T {
            if (global) {
                auto value = getGlobalFlag(i);
                suilAssert0(value && std::holds_alternative<double>(*value));
                return (T) (int) std::get<double>(*value);
            }
            else {
                auto value = getFlag(i);
                suilAssert0(value && std::holds_alternative<double>(*value));
                return (T) (int) std::get<double>(*value);
            }
        }

        template <typename T>
            requires (std::is_enum_v<T>)
        auto get(const char *name) -> T {
            auto value = getFlag(name);
            suilAssert0(value && std::holds_alternative<double>(*value));
            return (T) (int) std::get<double>(*value);
        }

        template <typename T>
            requires (std::is_enum_v<T>)
        auto at(uint32 i) -> T {
            auto value = getPositional(i);
            suilAssert0(value && std::holds_alternative<double>(*value));
            return (T) (int) std::get<double>(*value);
        }

        template <typename T>
            requires (std::is_same_v<T, bool>)
        auto get(uint32 i, bool global = false) -> bool {
            if (global) {
                auto value = getGlobalFlag(i);
                suilAssert0(value && std::holds_alternative<bool>(*value));
                return std::get<bool>(*value);
            }
            else {
                auto value = getFlag(i);
                suilAssert0(value && std::holds_alternative<bool>(*value));
                return std::get<bool>(*value);
            }
        }

        template <typename T>
            requires (std::is_same_v<T, bool>)
        auto get(const char *name) -> bool {
            auto value = getFlag(name);
            suilAssert0(value && std::holds_alternative<bool>(*value));
            return (T) std::get<bool>(*value);
        }

        template <typename T>
            requires (std::is_same_v<T, bool>)
        auto at(uint32 i) -> bool {
            auto value = getPositional(i);
            suilAssert0(value && std::holds_alternative<bool>(*value));
            return (T) std::get<bool>(*value);
        }

        template <typename T>
            requires (std::is_same_v<T, const char*>)
        auto get(uint32 i, bool global = false) -> const char* {
            if (global) {
                auto value = getGlobalFlag(i);
                suilAssert0(value && std::holds_alternative<const char*>(*value));
                return std::get<const char*>(*value);
            }
            else {
                auto value = getFlag(i);
                suilAssert0(value && std::holds_alternative<const char*>(*value));
                return std::get<const char*>(*value);
            }
        }

        template <typename T>
            requires (std::is_same_v<T, const char*>)
        auto get(const char *name) -> const char* {
            auto value = getFlag(name);
            suilAssert0(value && std::holds_alternative<const char*>(*value));
            return (T) std::get<const char*>(*value);
        }

        template <typename T>
            requires (std::is_same_v<T, const char*>)
        auto at(uint32 i) -> const char* {
            auto value = getPositional(i);
            suilAssert0(value && std::holds_alternative<const char*>(*value));
            return (T) std::get<const char*>(*value);
        }
    };

    struct CmdParser {
        const char *name{nullptr};
        const char *version{nullptr};
        const char *describe{nullptr};
        Command *def{nullptr};
        uint16         la{0};
        uint16         lc{0};
        uint32         ncmds{0};
        uint32         nargs{0};
        Command **cmds{nullptr};
        CmdFlag     *args{nullptr};
        char         error[128] = {0};
    };

    struct CmdEnumDesc {
        const char *name;
        uint32 value;
    };

    bool cmdParseString(CmdParser *P, CmdFlagValue* dst, const char *str, const char *name);
    bool cmdParseBoolean(CmdParser *P, CmdFlagValue* dst, const char *str, const char *name);
    bool cmdParseDouble(CmdParser *P, CmdFlagValue* dst, const char *str, const char *name);
    bool cmdParseInteger(CmdParser *P, CmdFlagValue* dst, const char *str, const char *name);
    bool cmdParseByteSize(CmdParser *P, CmdFlagValue* dst, const char *str, const char *name);
    bool cmdParseBitFlags(CmdParser *P,
                        CmdFlagValue* dst,
                        const char *str,
                        const char *name,
                        CmdEnumDesc *bitFlags,
                        uint32 len);
    bool cmdParseEnumValue(CmdParser *P,
                        CmdFlagValue* dst,
                        const char *str,
                        const char *name,
                        const CmdEnumDesc *enumDesc,
                        uint32 len);

    static bool cmdParseLogVerbosity(CmdParser *P, CmdFlagValue* dst, const char *str, const char *name)
    {
        const CmdEnumDesc sLOG_VERBOSITY[] = {
#if SUIL_ENABLE_TRACE==1
            {"TRACE", TRACE},
#endif      
            {"DEBUG", DEBUG},
            {"INFO",  INFO},
            {"NOTICE", NOTICE},
            {"WARNING", WARNING},
            {"ERROR", ERROR},
            {"CRITICAL", CRITICAL},
        };
        return cmdParseEnumValue(P,
                                dst,
                                str,
                                name,
                                sLOG_VERBOSITY,
                                sizeof(sLOG_VERBOSITY)/sizeof(CmdEnumDesc));
    }

    bool cmdPromptPassword(Command *cmd, CmdFlag *flag, const void* ctx);
    bool cmdPromptWith(Command *cmd, CmdFlag *flag, const void* ctx);
    bool cmdPrompt(Command *cmd, CmdFlag *flag, const void* ctx);

#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_NAME)
    #define Name(N) .name = N
#endif
    #define SuilName(N) .name = N
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_SF)
    #define Sf(S) .sf = S
#endif
    #define SuilSf(S) .sf = S
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_HELP)
    #define Help(H) .help = H
#endif
    #define SuilHelp(H) .help = H
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_TYPE)
    #define Type(V) .validator = V
#endif
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_DEF)
    #define Def(D) .def = D
#endif
    #define SuilDef(D) .def = D
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_POSITIONALS)
    #define Positionals(...) { __VA_ARGS__ }
#endif
    #define SuilPositionals(...) { __VA_ARGS__ }
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_OPT)
    #define Opt(...)  {__VA_ARGS__, .validator = NULL}
#endif
    #define SuilOpt(...)  {__VA_ARGS__, .validator = NULL}
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_STR)
    #define Str(...)  {__VA_ARGS__, .validator = suil::cmdParseString}
#endif
    #define SuilStr(...)  {__VA_ARGS__, .validator = suil::cmdParseString}
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_INT)
    #define Int(...)  {__VA_ARGS__, .validator = suil::cmdParseInteger}
#endif
    #define SuilInt(...)  {__VA_ARGS__, .validator = suil::cmdParseInteger}
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_BOOL)
    #define Bool(...) {__VA_ARGS__, .validator = suil::cmdParseBoolean}
#endif
    #define SuilBool(...) {__VA_ARGS__, .validator = suil::cmdParseBoolean}
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_FLOAT)
    #define Float(...) {__VA_ARGS__, .validator = suil::cmdParseDouble}
#endif
    #define SuilFloat(...) {__VA_ARGS__, .validator = suil::cmdParseDouble}
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_BYTES)
    #define Bytes(...)  {__VA_ARGS__, .validator = suil::cmdParseByteSize}
#endif
    #define SuilBytes(...)  {__VA_ARGS__, .validator = suil::cmdParseByteSize}
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_VERBOSITY)
    #define Verbosity(...)  {__VA_ARGS__, .validator = suil::cmdParseLogVerbosity}
#endif
    #define SuilVerbosity(...)  {__VA_ARGS__, .validator = suil::cmdParseLogVerbosity}
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_USE)
    #define Use(V, ...) {__VA_ARGS__, .validator = V}
#endif
    #define SuilUse(V, ...) {__VA_ARGS__, .validator = V}
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_PROMPT)
    #define Prompt(P, ...) {__VA_ARGS__, .prompt = P}
#endif
    #define SuilPrompt(P, ...) {__VA_ARGS__, .prompt = P}
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_PROMPT_CTX)
    #define PromptCtx(ctx) .promptCtx = ctx
#endif
    #define SuilPromptCtx(ctx) .promptCtx = ctx
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFIX_PROMPT_PASSWORD)
    #define PromptPassword(...) Prompt(suil::cmdPromptPassword, __VA_ARGS__, PromptCtx("Enter password: "))
#endif
    #define SuilPromptPassword(...) Prompt(suil::cmdPromptPassword, __VA_ARGS__, PromptCtx("Enter password: "))
#if !defined(SUIL_CMD_PREFIX) && !defined(SUIL_CMD_PREFI_PROMPT_WITH)
    #define PromptWith(V, ...) Prompt(suil::cmdPromptWith, __VA_ARGS__, PromptCtx(V))
#endif
    #define SuilPromptWith(V, ...) Prompt(suil::cmdPromptWith, __VA_ARGS__, PromptCtx(V))

    #define Sizeof(T, ...) (sizeof((T[]){__VA_ARGS__})/sizeof(T))
    #define AddCommand(N, H, P, ...)                                                \
        static int CMD_##N = 0;                                                     \
        typedef struct Cmd##N Cmd##N;                                               \
        static int N##_Handler(suil::Command& cmd, int argc, char** argv);          \
        struct Cmd##N {                                                             \
            suil::Command meta;                                                     \
            suil::CmdFlag args[Sizeof(suil::CmdFlag, __VA_ARGS__)];                 \
            suil::CmdPositional pos[sizeof((suil::CmdPositional[])P)/               \
                                    sizeof(suil::CmdPositional)];                   \
        } N = {                                                                     \
            .meta = {                                                               \
                .name = #N,                                                         \
                .help = H,                                                          \
                .handler = N##_Handler,                                             \
                .nargs = Sizeof(suil::CmdFlag, __VA_ARGS__),                        \
                .npos  = (sizeof((suil::CmdPositional[])P)/                         \
                          sizeof(suil::CmdPositional))                              \
            },                                                                      \
            .args = {                                                               \
                __VA_ARGS__                                                         \
            },                                                                      \
            .pos = P                                                                \
        };                                                                          \
        int N##_Handler(suil::Command& cmd, int argc, char**argv)

     #define AddCommandLocal(N, HELP, HANDLER, P, ...)                              \
        static int CMD_##N = 0;                                                     \
        typedef struct Cmd##N Cmd##N;                                               \
        struct Cmd##N {                                                             \
            suil::Command meta;                                                     \
            suil::CmdFlag args[Sizeof(suil::CmdFlag, __VA_ARGS__)];                 \
            suil::CmdPositional pos[sizeof((suil::CmdPositional[])P)/               \
                                    sizeof(suil::CmdPositional)];                   \
        } N = {                                                                     \
            .meta = {                                                               \
                .name = #N,                                                         \
                .help = HELP,                                                       \
                .handler = HANDLER,                                                 \
                .nargs = Sizeof(suil::CmdFlag, __VA_ARGS__),                        \
                .npos  = (sizeof((suil::CmdPositional[])P)/                         \
                          sizeof(suil::CmdPositional))                              \
            },                                                                      \
            .args = {                                                               \
                __VA_ARGS__                                                         \
            },                                                                      \
            .pos = P                                                                \
        }

    #define AddCmd(N, ...) ([&cmdCOUNT]() -> suil::Command* {                       \
                            CMD_##N = cmdCOUNT++; (N).meta.id = CMD_##N;            \
                            (N).meta.args = (N).args;                               \
                            (N).meta.pos = (N).pos;                                 \
                            return &((N).meta);                                     \
                        }())

    #define AddCmdCapture(N) ([&cmdCOUNT, &N]() -> suil::Command* {                 \
                            CMD_##N = cmdCOUNT++; (N).meta.id = CMD_##N;            \
                            (N).meta.args = (N).args;                               \
                            (N).meta.pos = (N).pos;                                 \
                            return &((N).meta);                                     \
                        }())

    

    void cmdShowUsage(CmdParser *P, const char *name, FILE *fp);
    int32  parseCommandLineArguments_(int *pargc,
                                    char ***pargv,
                                    CmdParser *P);

    #define RequireCmd NULL
    #define DefaultCmd(C) (&((C).meta))
    #define Commands(...) { AddCmdCapture(help), ##__VA_ARGS__ }


    #define CMDL_HELP_CMD                                                                           \
    AddCommandLocal(help, "Get the application or help related to a specific command", nullptr,     \
            Positionals(                                                                            \
                SuilStr(SuilName("command"),                                                        \
                        SuilHelp("The command whose help should be retrieved"),                     \
                        SuilDef(""))                                                                \
            )                                                                                       \
    );

    #define SuilParser(N, V, CMDS, DEF, ...)                                 \
        CMDL_HELP_CMD                                                       \
        int cmdCOUNT = 0;                                                   \
        struct {                                                            \
            suil::CmdParser P;                                              \
            suil::Command *cmds[(sizeof((suil::Command*[])CMDS) /           \
                                sizeof(suil::Command*))];                   \
            suil::CmdFlag     args[2+ Sizeof(suil::CmdFlag, __VA_ARGS__)];  \
        } parser = {                                                        \
            .P = {                                                          \
                .name = N,                                                  \
                .version = V,                                               \
                .def = DEF,                                                 \
                .ncmds = (sizeof((suil::Command*[])CMDS) /                  \
                                sizeof(suil::Command*)),                    \
                .nargs = 2 + Sizeof(suil::CmdFlag, __VA_ARGS__)             \
            },                                                              \
            .cmds = CMDS,                                                   \
            .args = { SuilOpt(SuilName("version"), SuilSf('v'),             \
                        SuilHelp("Show the application version"),           \
                        .isAppOnly = true),                                 \
                    SuilOpt(SuilName("help"), SuilSf('h'),                  \
                        SuilHelp("Get help for the selected command")),     \
                    ##__VA_ARGS__                                           \
                    }                                                       \
        };                                                                  \
        suil::CmdParser *P = &parser.P

    #define argparse(ARGC, ARGV, PP)                                        \
        ({ (PP).P.cmds = (PP).cmds; (PP).P.args = (PP).args;                \
            suil::parseCommandLineArguments_((ARGC), (ARGV),&(PP).P); })

    template <typename PP>
    int cmdExecute(PP& parser, int argc, char **argv)
    {
        int selected = argparse(&argc, &argv, parser);
        if (selected < 0) {
            fputs(parser.P.error, stderr);
            return EXIT_FAILURE;
        }

        auto cmd = parser.P.cmds[selected];
        suilAssert(cmd->handler != nullptr,
                "error: Command '%s' does not have a handler, was command added with `AddCommandLocal`?",
                cmd->name);
        
        return cmd->handler(*cmd, argc, argv);
    }
}
