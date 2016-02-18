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

#include <associative_array.h>
#include "naming_utils.h"
#include "tracing_utils.h"
#include "die.h"

#ifndef __x86_64
#error Only x86-64 is supported now
#endif

#define max(a, b) ((a) > (b) ? (a) : (b))

#define UNUSED(x) x __attribute__((unused))

struct {
    bool  was_called_null;
    void* current_break;
    void* break_beginning;
    uint64_t max_brk_mem;
} brk_info;

void brk_init(void) {
    brk_info.was_called_null = false;
    brk_info.current_break = NULL;
    brk_info.break_beginning = NULL;
    brk_info.max_brk_mem = 0;
}

void on_brk_enter(pid_t pid, uint64_t addr) {
    if (!brk_info.was_called_null && (void*)addr != NULL) {
        ptrace(PTRACE_KILL, pid, NULL, NULL);
        die(1, "Tracee attempted to call brk(%p) without previous brk(NULL). Killed.\n", (void*)addr);
    }
    if (!brk_info.was_called_null)
        brk_info.was_called_null = true;
}

void on_brk_leave(pid_t pid, uint64_t result) {
    assert(pid);
    if (result < (unsigned long long)(-4095)) {
        if (brk_info.current_break == NULL) {
            brk_info.current_break = (void*)result;
            brk_info.break_beginning = (void*)result;
        } else {
            brk_info.current_break = (void*)result;
            brk_info.max_brk_mem = max(brk_info.max_brk_mem, result - (uint64_t)brk_info.break_beginning);
        }
    }
}

int ptr_cmp(const void* a, const void* b)
{ // 0_0 ???
    if ((*((void**) a)) > (*((void**) b)))
        return 1;
    else if ((*((void**) a)) == (*((void**) b)))
        return 0;
    return -1;
}


struct {
    associative_array mmap_segments;
    uint64_t max_mmap_mem;
    size_t last_mmap_length;
} mmap_info;

struct userdata {
    register_type syscall_id;
};

void mmap_init(void) {
    mmap_info.mmap_segments = NULL;
    mmap_info.max_mmap_mem = 0;
}

void on_mmap_enter(pid_t UNUSED(pid), struct user_regs_struct regs) {
    mmap_info.last_mmap_length = regs.rsi;
}

void on_mmap_leave(pid_t UNUSED(pid), struct user_regs_struct regs) {
    if (regs.rsi < (unsigned long long)(-4095)) {
//        associative_array_add(mmap_info.mmap_segments,
//                              associative_array_add_init(sizeof(void*), sizeof(size_t), ptr_cmp, &((void*)regs.rax), &(mmap_info.last_mmap_length)));
    }
}

void on_munmap_enter(pid_t UNUSED(pid), struct user_regs_struct UNUSED(regs)) {

}

void on_munmap_leave(pid_t UNUSED(pid), struct user_regs_struct UNUSED(regs)) {

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
    
        if (info.id == SYS_brk)
            on_brk_enter(pid, info.arg1);
        
        fprintf(stderr, "Entering syscall %s(%lld, %lld, %lld, %lld, %lld, %lld)\n", buf, info.arg1, info.arg2, info.arg3, info.arg4, info.arg5, info.arg6);
    } else {
        extract_syscall_result(&regs, &info);
        if (info.err == 0)
            fprintf(stderr, "Leaving syscall, result %lld\n", info.ret);
        else
            fprintf(stderr, "Leaving syscall, error %lld\n", info.err);
        if (data->syscall_id == SYS_brk) {
            on_brk_leave(pid, regs.rax);
        }
    }
}

void on_child_exit(pid_t pid, int code, void* UNUSED(user_ptr)) {
    fprintf(stderr, "[%d exited with %d]\n", pid, code);
    fprintf(stderr, "\nMemory statistics:\n");
    fprintf(stderr, "Maximum brk(2) memory: %lu bytes\n", brk_info.max_brk_mem);
    fprintf(stderr, "Maximum mmap(2) memory: <unsupported yet>\n");
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
        
        brk_init();

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
