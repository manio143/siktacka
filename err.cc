#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>

#include "err.h"

void err(const char* fmt, ...) {
    va_list fmt_args;

    fprintf(stderr, "ERROR: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
    exit(EXIT_FAILURE);
}
void debug(const char* fmt, ...) {
#if DEBUG
    va_list fmt_args;

    fprintf(stdout, "DEBUG: ");
    va_start(fmt_args, fmt);
    vfprintf(stdout, fmt, fmt_args);
    va_end(fmt_args);
#endif
}