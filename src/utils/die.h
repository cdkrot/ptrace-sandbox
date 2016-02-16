#ifndef PT_UTIL_DIE_H
#define PT_UTIL_DIE_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

inline __attribute__((noreturn)) void die(int exit_code, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "Call trace:\n");
    void* buf[100];
    int sz = backtrace(buf, 100);
    for (int i = 0, j = sz - 1; i < j; ++i, --j) {
        void* tmp = buf[i];
        buf[i] = buf[j], buf[j] = tmp;
    }
    backtrace_symbols_fd(buf, sz, 2);

    exit(exit_code);
}

inline void check_errno(int exit_code) {
    if (errno != 0)
        die(exit_code, "Unexpected error: %s\n", strerror(errno));
}
#endif
