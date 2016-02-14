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

#ifndef __x86_64
#error Only x86-64 is supported now
#endif

#define max(a, b) ((a) > (b) ? (a) : (b))

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

void mmap_init(void) {
    mmap_info.mmap_segments = NULL;
    mmap_info.max_mmap_mem = 0;
}

void on_mmap_enter(pid_t pid, struct user_regs_struct regs) {
    mmap_info.last_mmap_length = regs.rsi;
}

void on_mmap_leave(pid_t pid, struct user_regs_struct regs) {
    if (regs.rsi < (unsigned long long)(-4095)) {
//        associative_array_add(mmap_info.mmap_segments,
//                              associative_array_add_init(sizeof(void*), sizeof(size_t), ptr_cmp, &((void*)regs.rax), &(mmap_info.last_mmap_length)));
    }
}

void on_munmap_enter(pid_t pid, struct user_regs_struct regs) {

}

void on_munmap_leave(pid_t pid, struct user_regs_struct regs) {

}

void on_signal(pid_t pid, int sig) {
    fprintf(stderr, "Signal: %d in %d\n", sig, (int)(pid));
}

void on_syscall_enter(pid_t pid, int64_t* syscall_num) {
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    *syscall_num = regs.orig_rax;

    const char* syscall_name = get_syscall_name(regs.orig_rax);
    char buf[100];
    snprintf(buf, sizeof(buf), "%s{%lld}", (syscall_name == NULL ? "unknown" : syscall_name), regs.orig_rax);
    
    if ((*syscall_num) == SYS_brk)
        on_brk_enter(pid, regs.rdi);

    fprintf(stderr, "Entering syscall %s(%lld, %lld, %lld, %lld, %lld, %lld)\n", buf, regs.rdi, regs.rsi, regs.rdx, regs.r10, regs.r8, regs.r9);
}

void on_syscall_leave(pid_t pid, int64_t syscall_num) {
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, NULL, &regs);

    if (sizeof(long) != 8)
        die(1, "Sorry\n");
    
    // negative value in [-4095, -1] means negated errno, otherways it means "noerror", but result.
    if (regs.rax >= (unsigned long long)(-4095))
        fprintf(stderr, "Leaving syscall, error: %lld\n", -regs.rax);
    else
        fprintf(stderr, "Leaving syscall, result: %lld\n", regs.rax);

    if (syscall_num == SYS_brk)
        on_brk_leave(pid, regs.rax);
}

void on_child_exit(pid_t pid, int code) {
    fprintf(stderr, "[%d exited with %d]\n", pid, code);
    fprintf(stderr, "\nMemory statistics:\n");
    fprintf(stderr, "Maximum brk(2) memory: %lu bytes\n", brk_info.max_brk_mem);
    fprintf(stderr, "Maximum mmap(2) memory: <unsupported yet>\n");
}

void on_groupstop(pid_t pid) {
    fprintf(stderr, "Group stop %d\n", (int)(pid));
}

void check_errno() {
    if (errno != 0)
        die(1, "Got error: %s\n", strerror(errno));
}

int main(int argc, char** argv) {
    if (argc < 2)
        die(1, "Not enough args, wanted: >=1, %d supplied\n", argc - 1);

    pid_t child = fork();
    if (child == -1)
        die(1, "Failed to create process\n");
    if (child == 0) {
        // child code.
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);

        
        // we are highly interested in setuping some custom ptrace options,
        // but this setup is possible only when ptrace already delivered some
        // event.
        // since this custom options are containing security ones we want to
        // do it as fast, as possible (so issue a syscall, which will allow tracer
        // to do this setup).
        struct timespec tm;
        tm.tv_sec = 0, tm.tv_nsec = 10;
        nanosleep(&tm, NULL);
        
        execve(argv[1], argv + 1, NULL);
    } else {
        // parent code.
        
        brk_init();

        int insyscall = 0;
        int options_enabled = 0;
        int64_t syscall_num;
        while (1) {
            int status;
            child = waitpid((pid_t)(-1), &status, __WALL);
            if (errno == ECHILD)
                break;
            
            check_errno();

            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                on_child_exit(child, WEXITSTATUS(status));
                continue;
            }
            
            if (!WIFSTOPPED(status)) {
                die(1, "Unknown event\n");
            }
            int stopsig = WSTOPSIG(status);
            int siginfo_queried = 0;
            siginfo_t sig;
            
            int syscallstop = 0;            
            if (options_enabled && stopsig == (SIGTRAP | 0x80))
                syscallstop = 1;
            if (! options_enabled && stopsig == (SIGTRAP)) {
                // potential syscall stop, need investigation.
                if (!siginfo_queried) {
                    siginfo_queried = 1;
                    ptrace(PTRACE_GETSIGINFO, child, 0, &sig);
                    check_errno();
                }
                syscallstop = (sig.si_code == SIGTRAP || sig.si_code == (SIGTRAP | 0x80));
            }

            if (syscallstop) {
                if (!insyscall)
                    on_syscall_enter(child, &syscall_num), insyscall = 1;
                else
                    on_syscall_leave(child, syscall_num), insyscall = 0;
                if (!options_enabled) {
                    ptrace(PTRACE_SETOPTIONS, child, NULL,
                           PTRACE_O_TRACEEXEC | PTRACE_O_TRACEEXIT | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACESYSGOOD | PTRACE_O_EXITKILL);
                    options_enabled = 1;
                    check_errno();
                }
                ptrace(PTRACE_SYSCALL, child, NULL, NULL);
                continue;
            }
            
            if (stopsig == SIGSTOP || stopsig == SIGTSTP || stopsig == SIGTTIN || stopsig == SIGTTOU) {
                // potential group-stop.
                if (!siginfo_queried) {
                    ptrace(PTRACE_GETSIGINFO, child, 0, &sig);
                    siginfo_queried = 1;
                }
                
                if (errno == EINVAL) {
                    errno = 0;
                    on_groupstop(child);
                    ptrace(PTRACE_SYSCALL, child, NULL, NULL);
                    continue;
                }
            }

            if (WSTOPSIG(status) == SIGTRAP) {
                // potential PTRACE_EVENT.
                int ptrace_event = 0;
                switch (status >> 8) {
                    case (SIGTRAP | (PTRACE_EVENT_VFORK << 8)):
                    case (SIGTRAP | (PTRACE_EVENT_FORK << 8)):
                    case (SIGTRAP | (PTRACE_EVENT_CLONE << 8)):
                    case (SIGTRAP | (PTRACE_EVENT_VFORK_DONE << 8)):
                    case (SIGTRAP | (PTRACE_EVENT_EXEC << 8)):
                    case (SIGTRAP | (PTRACE_EVENT_EXIT << 8)):
#ifdef PTRACE_EVENT_STOP
                    case (SIGTRAP | (PTRACE_EVENT_STOP << 8)):
#endif
                    case (SIGTRAP | (PTRACE_EVENT_SECCOMP << 8)):
                        ptrace_event = 1;
                }
                
                if (ptrace_event) {
                    fprintf(stderr, "Ignoring PTRACE_EVENT_...\n");
                    ptrace(PTRACE_SYSCALL, child, NULL, NULL);
                    continue;
                }
            }
            
            on_signal(child, stopsig);
            // For unknown reason detector detects weird SIGTRAP event,
            // at the very begining of executions, further investigations lead
            // to ptrace manpage, saying: that "SIGTRAP was delivered as a result of userspace action..."
            // [See it yourself].
            // Anyway, delivering this siganl results in tracee instant death.
            // so don't deliver SIGTRAP's now.
            ptrace(PTRACE_SYSCALL, child, NULL, (stopsig == SIGTRAP ? 0 : stopsig));
        }
    }
    
    return 0;
}
