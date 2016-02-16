#include "tracing_utils.h"
#include "die.h"
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>

// TODO: move architecture-dependent code in separate file or module
#if defined(__x86_64)

void extract_registers(pid_t ch, struct user_regs_struct* regs) {
    ptrace(PTRACE_GETREGS, ch, NULL, regs);
}

void extract_syscall_params(const struct user_regs_struct* regs, struct syscall_info* out) {
    out->id   = regs->orig_rax;
    out->arg1 = regs->rdi;
    out->arg2 = regs->rsi;
    out->arg3 = regs->rdx;
    out->arg4 = regs->r10;
    out->arg5 = regs->r8;
    out->arg6 = regs->r9;
}

int is_negated_errno(register_type code) {
    return code >= (register_type)(-4095);
    // yep, it's magic number.
    // please consult glibc and linux kernel for explanations
}

void extract_syscall_result(const struct user_regs_struct* regs, struct syscall_info* out) {
    if (is_negated_errno(regs->rax))
        out->err = -regs->rax;
    else
        out->err = 0, out->ret = regs->rax;
}

#endif

void trace_me() {
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);

    // we are highly interested in setuping some custom ptrace options,
    // but this setup is possible only when ptrace already delivered some
    // event.
    // since this custom options are containing security ones we want to
    // do it as fast, as possible (so issue a syscall, which will allow tracer
    // to do this setup).

    // TODO: unfortunately doesn't track any syscalls before execve
    struct timespec tm;
    tm.tv_sec = 0, tm.tv_nsec = 10;
    nanosleep(&tm, NULL);
}

void tracing_loop(const struct tracing_callbacks* callbacks, void* userdata) {
    // TODO: setup options of child processes too.
    
    int insyscall = 0;
    int options_enabled = 0;
    while (1) {
        int status;
        pid_t child = waitpid((pid_t)(-1), &status, __WALL);
        if (errno == ECHILD) {
            errno = 0;
            break;
        }
        
        check_errno(1);
        
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            callbacks->on_child_exit(child, WEXITSTATUS(status), userdata);
            continue;
        }
            
        if (!WIFSTOPPED(status))
            die(1, "Unknown event\n");
        
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
                check_errno(1);
            }
            syscallstop = (sig.si_code == SIGTRAP || sig.si_code == (SIGTRAP | 0x80));
        }

        if (syscallstop) {
            callbacks->on_syscall(child, insyscall, userdata), insyscall = !insyscall;
            if (!options_enabled) {
                ptrace(PTRACE_SETOPTIONS, child, NULL,
                       PTRACE_O_TRACEEXEC | PTRACE_O_TRACEEXIT | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE | PTRACE_O_TRACESYSGOOD | PTRACE_O_EXITKILL);
                options_enabled = 1;
                check_errno(1);
            }
            ptrace(PTRACE_SYSCALL, child, NULL, NULL);
            continue;
        }

        insyscall = 0;

        if (stopsig == SIGSTOP || stopsig == SIGTSTP || stopsig == SIGTTIN || stopsig == SIGTTOU) {
            // potential group-stop.
            if (!siginfo_queried) {
                ptrace(PTRACE_GETSIGINFO, child, 0, &sig);
                siginfo_queried = 1;
            }
                
            if (errno == EINVAL) {
                errno = 0;
                callbacks->on_groupstop(child, userdata);
                ptrace(PTRACE_SYSCALL, child, NULL, NULL);
                continue;
            }
        }

        if (WSTOPSIG(status) == SIGTRAP) {
            // potential PTRACE_EVENT.
            int ptrace_event = 0;
#define GEN_PTRACE_EVENT_HANDLER(EVENT)        \
            case (SIGTRAP | (EVENT << 8)):     \
                ptrace_event = EVENT;          \
                break;                         \
                
            switch (status >> 8) {
                GEN_PTRACE_EVENT_HANDLER(PTRACE_EVENT_VFORK);
                GEN_PTRACE_EVENT_HANDLER(PTRACE_EVENT_FORK);
                GEN_PTRACE_EVENT_HANDLER(PTRACE_EVENT_CLONE);
                GEN_PTRACE_EVENT_HANDLER(PTRACE_EVENT_VFORK_DONE);
                GEN_PTRACE_EVENT_HANDLER(PTRACE_EVENT_EXEC);
                GEN_PTRACE_EVENT_HANDLER(PTRACE_EVENT_EXIT);
#ifdef PTRACE_EVENT_STOP
                GEN_PTRACE_EVENT_HANDLER(PTRACE_EVENT_STOP);
#endif
                GEN_PTRACE_EVENT_HANDLER(PTRACE_EVENT_SECCOMP);
            }
            
                if (ptrace_event) {
                    // TODO: More complex handling here.
                    // For example we want to set options for newly created processes and so on.
                    callbacks->on_ptrace_event(child, ptrace_event, userdata);
                    ptrace(PTRACE_SYSCALL, child, NULL, NULL);
                    continue;
                }
            }
            
        if (callbacks->on_signal(child, stopsig, userdata))
            ptrace(PTRACE_SYSCALL, child, NULL, stopsig);
        else
            ptrace(PTRACE_SYSCALL, child, NULL, 0);
    }
}
