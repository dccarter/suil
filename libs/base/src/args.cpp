/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-21
 */

#include "suil/base/args.hpp"

#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/ioctl.h>
#endif

namespace {

#if !defined(__unix__) && !defined(__APPLE__)
    inline
#endif
    uint32 cmdTerminalColumns(FILE *fp)
    {
#if defined(__unix__) || defined(__APPLE__)

        struct winsize ws;
        int fd = fileno(fp);
        if (fd >= 0 && ioctl(fd, TIOCGWINSZ, &ws) == 0)
            return std::max(uint32(80), uint32(ws.ws_col));
#endif
        return 80;
    }

    bool cmdAddIndented(FILE *fp, uint32 ident, uint32 col, const char *text, uint32 added)
    {
        if (col < 20) return false;
        if (text == nullptr || text[0] == '\0') return true;

        if (added == 0)
            fprintf(fp, "%*c", ident, ' ');
        else if (added == -1) added = 0;

        for (uint32 i = ident; (text != nullptr && text[0] != '\0'); ) {
            const char *sp = strchr(text, ' ');
            const uint32 len = (sp != nullptr)? (++sp - text) : strlen(text);
            if (i + len > (col - added)) {
                // jump to the next line
                fputc('\n', fp);
                return cmdAddIndented(fp, ident, col, text, 0);
            }
            fwrite(text, sizeof(text[0]), len, fp);
            i += len;
            text = sp;
        }

        return true;
    }

    inline
    bool cmdIsLongFlag(const char *s)
    {
        return (s[0] == '-' && s[1] == '-' && s[2] != '\0');
    }

    inline
    bool cmdIsShortFlag(const char *s)
    {
        return (s[0] == '-' &&  s[1] != '-' && s[1] != '\0');
    }

    inline
    bool cmdIsIgnoreArguments(const char *s)
    {
        return (s[0] == '-' && s[1] == '-' && s[2] == '\0');
    }

    suil::CmdFlag* cmdFindByName(suil::CmdFlag *args, uint32 nargs, const char *name, uint32 len)
    {
        for (int i = 0; i < nargs; i++) {
            if (args[i].name == nullptr) continue;
            if (strncmp(args[i].name, name, len) == 0)
                return &args[i];
        }

        return nullptr;
    }

    suil::CmdFlag* cmdFindByShortFormat(suil::CmdFlag *args, uint32 nargs, char sf)
    {
        for (int i = 0; i < nargs; i++) {
            if (args[i].sf == '\0') continue;
            if (args[i].sf == sf)
                return &args[i];
        }

        return nullptr;
    }

    suil::Command* cmdFindCommandByName(suil::CmdParser *P, const char *name)
    {
        if (name == nullptr) return nullptr;
        if (name[0] == '\0') return P->def;

        for (int i = 0; i < P->ncmds; i++) {
            if (P->cmds[i]->name == nullptr) continue;
            if (strcmp(P->cmds[i]->name, name) == 0)
                return P->cmds[i];
        }

        return nullptr;
    }

    void cmdShowCommandFlag(const suil::CmdFlag *arg, uint16 la, uint32 cols, FILE *fp, bool skipStar)
    {
        uint32 ident;
        int32 defLen = -1;
        int spaces = std::max(0, int(la - strlen(arg->name))) + 2;
        ident = spaces;
        spaces += (arg->sf == '\0')? 4 : 0;
        ident  += 4;

        fprintf(fp, "%*c", spaces, ' ');

        if (!skipStar) {
            if (arg->validator && arg->def == nullptr)
                fputc('*', fp);
            else
                fputc(' ', fp);
            ident++;
        }

        if (!arg->prompt) {
            if (arg->sf != '\0')
                fprintf(fp, "-%c, ", arg->sf);
            fprintf(fp, "--%s", arg->name);
        }
        else {
            fprintf(fp, "  %s", arg->name);
        }

        ident += strlen(arg->name) + 2;

        if (arg->validator)
            fputc('=', fp);
        else if (arg->prompt)
            fputc('#', fp);
        else
            fputc(' ', fp);
        fputc(' ', fp);
        ident += 2;

        if (arg->prompt || (arg->def && arg->def[0] != '\0')) {
            fputc('(', fp);
            defLen = 1;
        }

        if (arg->prompt ) {
            fputs("Prompt", fp);
            defLen += 6;
        }
        if (arg->def && arg->def[0] != '\0') {
            if (arg->prompt) {
                fputs(", ", fp);
                defLen += 2;
            }

            fprintf(fp, "Default: %s", arg->def);
            defLen += ((int32)strlen(arg->def) + 9);
        }
        
        if (arg->prompt || (arg->def && arg->def[0] != '\0')) {
            fputs(") ", fp);
            defLen += 2;
        }

        if (arg->help)
            cmdAddIndented(fp, ident, cols, arg->help, defLen);
        fputc('\n', fp);
    }

    void cmdShowCommandUsage(const suil::CmdParser *P, const suil::Command *cmd, uint32 cols, FILE *fp)
    {
        fprintf(fp, "Usage: %s %s", P->name, cmd->name);
        if (cmd->nargs || P->nargs)
            fputs(" [options]", fp);
        if (cmd->npos)
            fputs(" ...", fp);
        fputs("\n\n", fp);

        if (cmd->npos) {
            fputs("Command inputs (in order):\n", fp);
            for (int i = 0; i < cmd->npos; i++) {
                suil::CmdPositional *pos = &cmd->pos[i];
                int spaces = std::max(0, int(cmd->lp - strlen(pos->name))) + 2;
                uint32 ident = spaces;
                int32 defLen = -1;

                fprintf(fp, "%*c", spaces, ' ');
                if (pos->validator && pos->def == nullptr)
                    fputc('*', fp);
                else
                    fputc(' ', fp);
                fputs(pos->name, fp);
                fputc(' ', fp);
                ident += strlen(pos->name) + 2;

                if (pos->def  && pos->def[0] != '\0') {
                    fprintf(fp, "(Default: %s) ", pos->def);
                    defLen = (int32)strlen(pos->def) + 12;
                }

                if (pos->help)
                    cmdAddIndented(fp, ident, cols, pos->help, defLen);
                fputc('\n', fp);
            }
        }
        if (cmd->nargs) {
            fputs("\nCommand options:\n", fp);
            for (int i = 0; i < cmd->nargs; i++) {
                cmdShowCommandFlag(&cmd->args[i], cmd->la, cols, fp, false);
            }
        }
    }

    void initializeCommand(suil::Command *cmd, suil::CmdFlag *gArgs, uint32 ngArgs)
    {
        for (int i = 0; i < cmd->nargs; i++) {
            suil::CmdFlag *arg = &cmd->args[i];
            memset(&arg->val, 0, sizeof(arg->val));

            for (int j = i+1; j < cmd->nargs; j++) {
                if (strcmp(arg->name, cmd->args[j].name) == 0) {
                    fprintf(stderr, "Duplicate duplicate flag name '%s' at index %d and %d on command '%s'!\n",
                                arg->name, i, j, cmd->name);
                    abort();
                }

                if (arg->sf != '\0' && arg->sf == cmd->args[j].sf) {
                    fprintf(stderr, "Duplicate flag short format '%c' detected at index %d and %d on command '%s'!\n",
                                arg->sf, i, j, cmd->name);
                    abort();
                }
            }

            for (int j = 0; j < ngArgs; j++) {
                if (strcmp(arg->name, gArgs[j].name) == 0) {
                    fprintf(stderr,
                                "Flag name '%s' defined in command '%s' at index %d "
                                "already used as global flag at index %d!\n",
                                arg->name, cmd->name, i, j);
                    abort();
                }

                if (arg->sf != '\0' && (arg->sf == gArgs[j].sf)) {
                    fprintf(stderr, "Flag short format '%c' defined in '%s' at index %d "
                                    "already used as global flag at index %d!\n",
                                    arg->sf, arg->name, i, j) ;
                    abort();
                }
            }

            cmd->la = std::max(cmd->la, uint16(strlen(cmd->args[i].name)));
        }

        for (int i = 0; i < cmd->npos; i++) {
            memset(&cmd->pos[i].val, 0, sizeof(suil::CmdFlagValue));
            cmd->lp = std::max(cmd->lp, (uint16)strlen(cmd->pos[i].name));
        }
    }

    bool cmdParseCommandArguments(suil::CmdParser *P, suil::Command *cmd, int *pargc, char ***pargv)
    {
        int argc = *pargc;
        char **argv = *pargv;
        uint32 npos = 0;

        for (; argc > 0; --argc, ++argv) {
            const char *arg = *argv;
            const char *og = arg;
            const char *val = nullptr;
            suil::CmdFlag *flag;

            if (cmdIsIgnoreArguments(arg)) {
                --argc;
                ++argv;
                break;
            }

            if (cmdIsShortFlag(arg)) {
                arg++;
                flag = cmdFindByShortFormat(P->args, P->nargs, arg[0]);
                if (flag == nullptr) flag = cmdFindByShortFormat(cmd->args, cmd->nargs, arg[0]);
                arg++;
                if (arg[0] != '\0')
                    val = arg;
            }
            else if (cmdIsLongFlag(arg)) {
                uint32 len;
                arg += 2;
                len = strlen(arg);
                const char *eq = strchr(arg, '=');
                if (eq) {
                    val = eq + 1;
                    while (isspace(*--eq));
                    len = eq - arg;
                    while (isspace(*val)) val++;
                }
                flag = cmdFindByName(P->args, P->nargs, arg, len);
                if (flag == nullptr) flag = cmdFindByName(cmd->args, cmd->nargs, arg, len);
            }
            else {
                if (npos >= cmd->npos) {
                    sprintf(cmd->P->error,
                            "error: unexpected number of positional arguments passed, expecting %u\n", cmd->npos);
                    return false;
                }
                suil::CmdPositional *pos = &cmd->pos[npos];
                if (!pos->validator) {
                    pos->val = arg;
                }
                else if (!pos->validator(P, &pos->val, arg, pos->name)) {
                    return false;
                }
                npos++;
                continue;
            }

            // parse argument value if any
            if (flag == nullptr) {
                sprintf(cmd->P->error,
                        "error: unrecognized command argument '%s'\n", og);
                return false;
            }

            if (flag->validator) {
                if (val == nullptr && argc > 0) {
                    val = *++argv;
                    --argc;
                }
                if (val == nullptr) {
                    sprintf(cmd->P->error,
                            "error: command flag '%s' expects a value\n", flag->name);
                    return false;
                }
                if (!flag->validator(P, &flag->val, val, flag->name)) {
                    return false;
                }
            }
            else if (val != nullptr) {
                if (flag->prompt)
                    sprintf(cmd->P->error,
                        "error: prompt flag '%s' cannot be passed via command line\n", flag->name);
                else
                    sprintf(cmd->P->error,
                            "error: unexpected value passed to flag '%s'\n", flag->name);
                return false;
            }
            else {
                // flag that doesn't take a value
                flag->val = true;
            }
        }

        *pargc = argc;
        *pargv = argv;
        return true;
    }

    void cmdHandleBuiltins(suil::Command& cmd, bool def)
    {
        suil::CmdParser *P = cmd.P;
        if (cmd.id == 0) {
            auto target = cmd.getPositional(0);
            cmdShowUsage(P, target? std::get<const char*>(*target): nullptr, stdout);
            exit(EXIT_SUCCESS);
        }
        else {
            auto version = cmd.getGlobalFlag(0);
            auto help = cmd.getGlobalFlag(1);
            if (version && std::get<bool>(*version)) {
                fprintf(stdout, "%s v%s\n", P->name, P->version);
                exit(EXIT_SUCCESS);
            }
            if (help && std::get<bool>(*help)) {
                cmdShowUsage(P, def? nullptr : cmd.name, stdout);
                exit(EXIT_SUCCESS);
            }
        }
    }

    bool cmdValidateArg(suil::Command* cmd, suil::CmdFlag *flag)
    {
        auto P = cmd->P;
        if (!std::holds_alternative<std::monostate>(flag->val))
            return true;

        if (flag->validator == nullptr) {
            // flags that do not have a validator are considered optional
            flag->val = false;
            return true;
        }

        if (flag->def != nullptr) {
            if (flag->def[0] != '\0' && !flag->validator(P, &flag->val, flag->def, flag->name))
                return false;
        }
        else {
            sprintf(P->error, "Missing flag '%s' required by command '%s'\n",
                            flag->name, cmd->name);
            return false;
        }
        return true;
    }
}

namespace suil {

    bool cmdParseString(CmdParser */*unused*/ ,
                        CmdFlagValue* dst,
                        const char *str,
                        const char */*unsued*/ )
    {
        *dst = str;
        return true;
    }

    bool cmdParseBoolean(CmdParser *P, CmdFlagValue* dst, const char *str, const char *name)
    {
        uint32 len = strlen(str);
        if (len == 1) {
            if (str[0] == '0' || str[0] == '1') {
                *dst = str[0] == '1';
            }
            else
                goto parseBoolError;
        }
        else if (len == 4 && strcasecmp(str, "true") == 0) {
            *dst = true;
        }
        else if (len == 5 && strcasecmp(str, "false") == 0) {
            *dst = false;
        } else
            goto parseBoolError;

        return true;

    parseBoolError:
        sprintf(P->error,
                "error: value '%s' passed to flag '%s' cannot be parsed as a boolean\n", str, name);
        return false;
    }

    bool cmdParseDouble(CmdParser *P, CmdFlagValue* dst, const char *str, const char *name)
    {
        char *end;
        *dst = strtod(str, &end);
        if (errno == ERANGE || *end != '\0') {
            sprintf(P->error,
                    "error: value '%s' passed to flag '%s' cannot be parsed as a double\n", str, name);
            return false;
        }

        return true;
    }

    bool cmdParseInteger(CmdParser *P, CmdFlagValue* dst, const char *str, const char *name)
    {
        char *end;
        *dst = (double)strtoll(str, &end, 0);
        if (errno == ERANGE || *end != '\0') {
            sprintf(P->error,
                    "error: value '%s' passed to flag '%s' cannot be parsed as a number\n", str, name);
            return false;
        }

        return true;
    }

    bool cmdParseByteSize(CmdParser *P, CmdFlagValue* dst, const char *str, const char *name)
    {
        char *end;
        auto size = (double) strtoull(str, &end, 0);
        if (errno == ERANGE) {
            sprintf(P->error,
                    "error: value '%s' passed to flag '%s' cannot be parsed as a number\n", str, name);
            return false;
        }

        if (end) {
            if (end[0] != '\0' && end[1] != '\0')
                goto unsupportedDataSizeUnit;

            switch (end[0]) {
                case 'g': case 'G':
                    *dst = size * 1024 * 1024 * 1024;
                    break;
                case 'm': case 'M':
                    *dst = size * 1024 * 1024;
                    break;
                case 'k': case 'K':
                    *dst = size *  1024;
                    break;
                case 'b': case 'B': case '\0':
                    break;
                default:
    unsupportedDataSizeUnit:
                    sprintf(P->error, "'%s' is not a supported data size unit "
                                    "(use g,G for gigabytes, m,M for megabytes, "
                                    "k,K for kilobytes or b, B for bytes)", end);
                    return false;
            }
        }

        return true;
    }

    bool cmdParseEnumValue(CmdParser *P,
                        CmdFlagValue* dst,
                        const char *str,
                        const char *name,
                        const CmdEnumDesc *enumDesc,
                        uint32 len)
    {
        for (int i = 0; i < len; i++) {
            if(strcasecmp(enumDesc[i].name, str) == 0) {
                *dst = double(enumDesc[i].value);
                return true;
            }   
        }

        sprintf(P->error, "'%s' is not a supported value for flag '%s'\n",
                            str, name);
        return false;
    }

    bool cmdParseBitFlags(CmdParser *P,
                        CmdFlagValue* dst,
                        const char *str,
                        const char *name,
                        CmdEnumDesc *bitFlags,
                        uint32 count)
    {
        uint32 value = 0;
        uint32 sizes[count];

        for (int i = 0; i < count; i++)
            sizes[i] = strlen(bitFlags[i].name);

        while (str) {
            bool parsed = false;
            const char *next;
            uint32 size = 0;
            while (*str && (*str == '|' || isspace(*str))) str++;
            next = strchr(str, '|');

            if (next != nullptr) {
                const char* e = next - 1;
                while (e != str && isspace(*e)) e--;
                if (e == str) continue;
                size = e - str + 1;
            }
            else {
                size = strlen(str);
            }

            for (int i = 0; i < count; i++) {
                if (size == sizes[i] && strncasecmp(str, bitFlags[i].name, size) == 0) {
                    value |= bitFlags[i].value;
                    parsed = true;
                    break;
                }
            }

            if (!parsed) {
                sprintf(P->error, "'%.*s' is not a supported bit flag for flag '%s'\n",
                                size, str, name);
                return false;
            }

            str = next;
        }
        *dst = (double) value;
        return true;
    }

    bool cmdPromptPassword(Command *cmd, CmdFlag *flag, const void* ctx)
    {
        if (ctx)
            flag->val = (const char*) getpass((const char *) ctx);
        else
            flag->val = (const char*) getpass("Enter password: ");
        

        return std::get<const char*>(flag->val) != nullptr;
    }

    std::string cmdReadStdin(CmdFlag *flag, const char *tag)
    {
        if (tag)
            fprintf(stdout, "%s", tag);
        else
            fprintf(stdout, "%s: ", flag->name);

        std::string data;
        std::getline(std::cin, data);
        return data;
    }

    bool cmdPrompt(Command *cmd, CmdFlag *flag, const void* ctx)
    {
        auto data = cmdReadStdin(flag, (const char*)ctx);
        if (!data.empty()) {
            flag->val = strdup(data.data());
            return true;
        }
        return false;
    }

    bool cmdPromptWith(Command *cmd, CmdFlag *flag, const void* ctx)
    {
        auto data = cmdReadStdin(flag, (const char*)ctx);
        if (!data.empty()) {
            char *val = strdup(data.data());
            if (ctx && (decltype(flag->validator)(ctx))(cmd->P, &flag->val, val, flag->name))
                return true;
            delete[] val;
        }

        return false;
    }

    std::optional<CmdFlagValue> Command::getFlag(const char *name)
    {
        const auto len = strlen(name);

        auto flag = cmdFindByName(P->args, P->nargs, name, len);
        if (flag == nullptr) flag = cmdFindByName(args, nargs, name, len);
        if (flag == nullptr || std::holds_alternative<std::monostate>(flag->val))
            return std::nullopt;

        return flag->val;
    }

    std::optional<CmdFlagValue> Command::getFlag(uint32 i)
    {
        if (i >= nargs || std::holds_alternative<std::monostate>(args[i].val))
            return std::nullopt;
        return args[i].val;
    }

    std::optional<CmdFlagValue> Command::getGlobalFlag(uint32 i)
    {
        if (P == nullptr || i >= P->nargs || std::holds_alternative<std::monostate>(P->args[i].val))
            return std::nullopt;
        return P->args[i].val;
    }

    std::optional<CmdFlagValue> Command::getPositional(uint32 i)
    {
        if (i >= npos || std::holds_alternative<std::monostate>(pos[i].val))
            return std::nullopt;
        return pos[i].val;
    }

    void cmdShowUsage(CmdParser *P, const char *name, FILE *fp)
    {
        uint32 cols = cmdTerminalColumns(fp);
        Command *cmd = cmdFindCommandByName(P, name);
        if (cmd != nullptr)
            cmdShowCommandUsage(P, cmd, cols, fp);
        else {
            fprintf(fp, "Usage: %s [cmd] ...\n\n", P->name);
            fputs("Commands:\n", fp);
            for (int i = 0; i < P->ncmds; i++) {
                cmd = P->cmds[i];
                int spaces = std::max(0, int(P->lc-strlen(cmd->name))) + 2;
                uint32 ident = spaces;
                fprintf(fp, "%*c", spaces, ' ');
                if (cmd == P->def)
                    fputc('*', fp);
                else
                    fputc(' ', fp);
                ident++;

                fputs(cmd->name, fp);
                fputc(' ', fp);
                ident += strlen(cmd->name) + 1;
                if (cmd->help)
                    cmdAddIndented(fp, ident, cols, cmd->help, -1);
                fputc('\n', fp);
            }
        }
        if (P->nargs) {
            fputs("\nGlobal Flags:\n", fp);
            for (int i = 0; i < P->nargs; i++) {
                const CmdFlag *arg = &P->args[i];
                if (arg->isAppOnly && cmd != nullptr) continue;
                cmdShowCommandFlag(arg, P->la, cols, fp, true);
            }
        }
    }

    int32  parseCommandLineArguments_(int *pargc,
                                    char ***pargv,
                                    CmdParser *P)
    {
        int argc = *pargc - 1;
        char **argv = *pargv + 1;

        for (int i = 0; i < P->nargs; i++) {
            CmdFlag *arg = &P->args[i];
            memset(&arg->val, 0, sizeof(arg->val));
            if (arg->prompt && arg->validator) {
                fprintf(stderr, "Flag '%s' is both prompt and argument, can only be either not both\n",
                                arg->name);
                    abort();
            }

            for (int j = i+1; j < P->nargs; j++) {
                if (strcmp(arg->name, P->args[j].name) == 0) {
                    fprintf(stderr, "Duplicate flag name'%s' detected at index %d and %d! on global flags!\n",
                                arg->name, i, j);
                    abort();
                }

                if (arg->sf != '\0' && arg->sf == P->args[j].sf) {
                    fprintf(stderr, "Duplicate flag short format '%c' detected at index %d and %d! on global flags!\n",
                                arg->sf, i, j);
                    abort();
                }
            }

            P->la = std::max(P->la, (uint16)strlen(P->args[i].name));
        }

        for (int i = 0; i < P->ncmds; i++) {
            // command names must be unique (slow but I don't suppose one will add 100 commands)
            for (int j = i + 1; j < P->ncmds; j++) {
                if (strcmp(P->cmds[i]->name, P->cmds[j]->name) == 0) {
                    fprintf(stderr, "Duplicate command name '%s'  detected at index %d and %d!\n",
                            P->cmds[i]->name, i, j);
                    abort();
                }
            }
            P->lc = std::max(P->lc, (uint16)strlen(P->cmds[i]->name));
            P->cmds[i]->P = P;
            initializeCommand(P->cmds[i], P->args, P->nargs);
        }

        Command *cmd = P->def;
        bool choseDef = true;
        if (argc) {
            if (argv[0][0] != '-') {
                if ((cmd = cmdFindCommandByName(P, argv[0]))) {
                    argv++;
                    argc--;
                    choseDef = false;
                } else
                    cmd = P->def;
            }
        }

        if (cmd == nullptr) {
            sprintf(P->error,
                    "error: missing required command to execute\n");
            return -1;
        }

        if (!cmdParseCommandArguments(P, cmd, &argc, &argv))
            return -1;

        cmdHandleBuiltins(*cmd, choseDef);

        // Validate that all required flags and positional arguments have been set
        std::vector<CmdFlag*> prompts;
        for (int i = 0; i < cmd->nargs; i++) {
            auto flag =  &cmd->args[i];
            if (flag->prompt) {
                prompts.push_back(flag);
                continue;
            }

            if (!cmdValidateArg(cmd, flag))
                return -1;
        }

        for (int i = 0; i < cmd->P->nargs; i++) {
            auto flag =  &cmd->P->args[i];
            if (flag->prompt) {
                prompts.push_back(flag);
                continue;
            }

            if (!cmdValidateArg(cmd, flag))
                return -1;
        }

        for (int i = 0; i < cmd->npos; i++) {
            CmdPositional *pos = &cmd->pos[i];
            if (!std::holds_alternative<std::monostate>(pos->val)) continue;

            if (pos->validator == nullptr)
                // flag that do not have a validator are considered optional
                continue;
            if (pos->def != nullptr) {
                if (pos->def[0] != '\0' && !pos->validator(P, &pos->val, pos->def, pos->name))
                    return -1;
            }
            else {
                sprintf(P->error, "Missing positional argument %d ('%s') required by command '%s'\n",
                        i, pos->name, cmd->name);
                return -1;
            }
        }

        for (auto flag: prompts) {
            if (!flag->prompt(cmd, flag, flag->promptCtx))
                return false;
        }

        *pargc = argc;
        *pargv = argv;

        return cmd->id;
    }

}
