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
