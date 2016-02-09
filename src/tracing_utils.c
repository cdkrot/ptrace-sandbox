#include "tracing_utils.h"
#include <sys/ptrace.h>
#include <sys/user.h>

void extract_syscall_params(pid_t child, struct syscall_info* out) {
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, child, 0, &regs);
#ifdef __x86_64
    out->id   = regs.orig_rax;
    out->arg1 = regs.rdi;
    out->arg2 = regs.rsi;
    out->arg3 = regs.rdx;
    out->arg4 = regs.r10;
    out->arg5 = regs.r8;
#else
#error "Other platforms are not supported yet"
#endif
}

void extract_syscall_result(pid_t child, struct syscall_info* out) {
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, child, 0, &regs);
#ifdef __x86_64
    out->ret = regs.rax;
#else
#error "Other platforms are not supported yet"
#endif
}

