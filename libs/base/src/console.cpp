//
// Created by Mpho Mbotho on 2020-10-12.
//

#include "suil/base/console.hpp"

#include <cstdio>


namespace suil::Console {
    void cprint(uint8_t color, int bold, const char *str) {
        if (color >= 0 && color <= CYAN)
            printf("\033[%s3%dm", (bold? "1;" : ""), color);
        (void) printf("%s", str);
        printf("\033[0m");
    }

    void cprintv(uint8_t color, int bold, const char *fmt, va_list args) {
        if (color > 0 && color <= CYAN)
            printf("\033[%s3%dm", (bold? "1;" : ""), color);
        (void) vprintf(fmt, args);
        printf("\033[0m");
    }

    void cprintf(uint8_t color, int bold, const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        cprintv(color, bold, fmt, args);
        va_end(args);
    }


    void println(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }

    void red(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        cprintv(RED, 0, fmt, args);
        va_end(args);
    }

    void blue(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        cprintv(BLUE, 0, fmt, args);
        va_end(args);
    }

    void green(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        cprintv(GREEN, 0, fmt, args);
        va_end(args);
    }

    void yellow(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        cprintv(YELLOW, 0, fmt, args);
        va_end(args);
    }

    void magenta(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        cprintv(MAGENTA, 0, fmt, args);
        va_end(args);
    }

    void cyan(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        cprintv(CYAN, 0, fmt, args);
        va_end(args);
    }

    void log(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        printf(fmt, args); printf("\n");
        printf("\n");
        va_end(args);
    }

    void info(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        cprintv(GREEN, 0, fmt, args);
        printf("\n");
        va_end(args);
    }

    void error(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        cprintv(RED, 0, fmt, args);
        printf("\n");
        va_end(args);
    }

    void warn(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        cprintv(YELLOW, 0, fmt, args);
        printf("\n");
        va_end(args);
    }

    class Input : public File {
    public:
        Input()
            : File(STDIN_FILENO)
        {}
    };

    File& in()
    {
        static Input sIn;
        return sIn;
    }
}