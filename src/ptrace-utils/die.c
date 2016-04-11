//  Different utils
//  Copyright (C) 2016  Sayutin Dmitry
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "die.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <execinfo.h>
#include <string.h>

__attribute__((noreturn)) void die(int exit_code, const char* fmt, ...) {
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

void check_errno(int exit_code) {
    if (errno != 0)
        die(exit_code, "Unexpected error: %s\n", strerror(errno));
}

