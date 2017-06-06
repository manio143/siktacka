#ifndef M_ERROR
#define M_ERROR
void err(const char* fmt, ...) {
    va_list fmt_args;

    fprintf(stderr, "ERROR: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
    // fprintf(stderr, " (%d; %s)\n", errno, strerror(errno));
    exit(EXIT_FAILURE);
}
#endif