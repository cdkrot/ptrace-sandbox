//  Ptrace-based code (detection of syscalls)
//  Copyright (C) 2016  Sayutin Dmitry, Vasiliy Alferov
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



#include <unistd.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <signal.h>
#include <time.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <execinfo.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>

#include "associative_array.h"
#include "naming_utils.h"
#include "tracing_utils.h"
#include "die.h"
#include "read_proc.h"

#ifndef __x86_64
#error Only x86-64 is supported now
#endif

#define max(a, b) ((a) > (b) ? (a) : (b))

#define UNUSED(x) x __attribute__((unused))

struct userdata {
    register_type syscall_id;
    unsigned long max_mem;
};

struct {
    unsigned long page_size;
} static_data;

void check_memory_usage(pid_t pid, void* data_) {
    struct userdata* data = (struct userdata*)data_;

    struct statm_info statm = get_process_statm_info(pid);
    data->max_mem = max(data->max_mem, statm.size * static_data.page_size);
}

void combined_name(char* buf, size_t buf_sz, long id, const char* (*get_name)(long)) {
    const char* friendlyname = get_name(id);
    snprintf(buf, buf_sz, "%s{%ld}", (friendlyname == NULL ? "unknown" : friendlyname), id);
}

int on_signal(pid_t pid, int sig, void* UNUSED(userptr)) {
    char buf[100];
    combined_name(buf, sizeof(buf), sig, get_signal_name);
    fprintf(stderr, "Signal: %s in %d\n", buf, (int)(pid));
    return (sig != SIGTRAP);
}

void on_syscall(pid_t pid, int type, void* data_) {
    struct user_regs_struct regs;
    struct syscall_info     info;
    struct userdata*        data = (struct userdata*)(data_);

    extract_registers(pid, &regs);

    if (type == 0) { // enter
        extract_syscall_params(&regs, &info);
        data->syscall_id = info.id;

        char buf[100];
        combined_name(buf, sizeof(buf), info.id, get_syscall_name);

        fprintf(stderr, "Entering syscall %s(%lld, %lld, %lld, %lld, %lld, %lld)\n", buf, info.arg1, info.arg2, info.arg3, info.arg4, info.arg5, info.arg6);
    } else {
        extract_syscall_result(&regs, &info);
        
        if (data->syscall_id == SYS_brk || data->syscall_id == SYS_mmap || data->syscall_id == SYS_munmap)
            check_memory_usage(pid, data_);

        if (info.err == 0)
            fprintf(stderr, "Leaving syscall, result %lld\n", info.ret);
        else
            fprintf(stderr, "Leaving syscall, error %lld\n", info.err);
    }
}

void on_child_exit(pid_t pid, int code, void* data_) {
    struct userdata* data = (struct userdata*)data_;
    fprintf(stderr, "[%d exited with %d]\n", pid, code);
    fprintf(stderr, "max memory used: %lu bytes (%lu kb)\n", data->max_mem, data->max_mem / 1024);
}

void on_groupstop(pid_t pid, void* UNUSED(user_ptr)) {
    fprintf(stderr, "Group stop %d\n", (int)(pid));
}

void on_ptrace_event(pid_t pid, int event, void* UNUSED(user_ptr)) {
    char buf[100];
    combined_name(buf, sizeof(buf), event, get_ptraceevent_name);
    fprintf(stderr, "Ptrace event %s in %d\n", buf, pid);
}

int main(int argc, char** argv) {
    if (argc < 2)
        die(1, "Not enough args, wanted: >=1, %d supplied\n", argc - 1);

    pid_t child = fork();
    if (child == -1)
        die(1, "Failed to create process\n");
    if (child == 0) {
        // child code.
        trace_me();
        
        execve(argv[1], argv + 1, NULL);
    } else {
        // parent code.
        
        static_data.page_size = getpagesize();

        struct tracing_callbacks callbacks;
        struct userdata data;
        callbacks.on_signal       = on_signal;
        callbacks.on_syscall      = on_syscall;
        callbacks.on_child_exit   = on_child_exit;
        callbacks.on_groupstop    = on_groupstop;
        callbacks.on_ptrace_event = on_ptrace_event;
        
        tracing_loop(&callbacks, &data); 
    }
    
    return 0;
}
