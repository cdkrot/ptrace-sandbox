//  Different utils
//  Copyright (C) 2016  Vasiliy Alferov
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

#ifndef PT_UTIL_READ_PROC_H
#define PT_UTIL_READ_PROC_H

#include <unistd.h>

/* This is a structure used to read values from /proc/[pid]/statm. Those values
 * are represented *in pages*. See proc(5) for details. */

struct statm_info {
    unsigned long size;     /* total program size (same as VmSize in /proc/[pid]/status) */
    unsigned long resident; /* resident set size (same as VmRSS in /proc/[pid]/status) */
    unsigned long share;    /* shared pages (i.e. backed by a file) */
    unsigned long text;     /* text (code) */
    unsigned long lib;      /* library (unused in Linux 2.6) */
    unsigned long data;     /* data + stack */
    unsigned long dt;       /* dirty pages (unused in Linux 2.6 */
};

struct statm_info get_process_statm_info(pid_t pid);

#endif //PT_UTIL_READ_PROC_H
