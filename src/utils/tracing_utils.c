#include "tracing_utils.h"
#include <sys/ptrace.h>
#include <sys/user.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

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

int is_negated_errorno(register_type code) {
    return code >= (register_type)(-4095);
    // yep, it's magic number.
    // please consult glibc and linux kernel for explanations
}

void extract_syscall_result(pid_t child, const struct user_regs_struct* regs, struct syscall_info* out) {
    // not implemented yet
    // See newdetect.c
}

#endif

