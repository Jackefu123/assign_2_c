#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include <stdio.h>
#include <stdlib.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define printf_red(format, ...) printf(ANSI_COLOR_RED format ANSI_COLOR_RESET, ##__VA_ARGS__)
#define printf_green(format, ...) printf(ANSI_COLOR_GREEN format ANSI_COLOR_RESET, ##__VA_ARGS__)
#define printf_yellow(format, ...) printf(ANSI_COLOR_YELLOW format ANSI_COLOR_RESET, ##__VA_ARGS__)

#define my_assert(expr)                                                                         \
    do                                                                                          \
    {                                                                                           \
        if (!(expr))                                                                            \
        {                                                                                       \
            printf_red("Assertion failed: %s, file %s, line %d.\n", #expr, __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                                                 \
        }                                                                                       \
    } while (0)

#endif // COMMON_DEFS_H
