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

void dump_registers(const struct user_regs_struct* regs) {
    fprintf(stderr, REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " "
                    REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " "
                    REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT "\n",    
            regs->r15,
            regs->r14,
            regs->r13,
            regs->r12,
            regs->rbp,
            regs->rbx,
            regs->r11,
            regs->r10,
            regs->r9,
            regs->r8,
            regs->rax,
            regs->rcx,
            regs->rdx,
            regs->rsi,
            regs->rdi,
            regs->orig_rax,
            regs->rip,
            regs->cs,
            regs->eflags,
            regs->rsp,
            regs->ss,
            regs->fs_base,
            regs->gs_base,
            regs->ds,
            regs->es,
            regs->fs,
            regs->gs);
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
    register_type rax = regs->rax;
    
    register_type data[80];
    data[61] = 100;
    size_t* pstack = (size_t*)(get_stack_pointer(regs));
    memcpy_from_proc(pstack, data, 80 * sizeof(register_type), child);

    out->ret = data[56];
    out->err = -rax;
}

void* get_stack_pointer(const struct user_regs_struct* regs) {
    return (void*)regs->rsp;
}
        
void memcpy_from_proc(void* src, void* dst, size_t n, pid_t child) {
    /* Don't know how linux kernel handles unaligned memory here
       So, please supply only aligned requests */
    /* Also, it DOESN't handle requests which size is not multiple of
       machine word */
    /* You are warned */
    /* TODO */
    assert(n % sizeof(size_t) == 0);
    assert(sizeof(size_t) == sizeof(size_t*));
    assert(sizeof(register_type) == sizeof(size_t));
    memset(dst, 0, n);
    
    size_t* dst_w = (size_t*)dst;
    size_t* src_w = (size_t*)src;
    for (size_t i = 0; i != n / sizeof(size_t); ++i)
        *(dst_w + i) = ptrace(PTRACE_PEEKDATA, child, (src_w + i), NULL);
}

void memcpy_to_proc(void* src, void* dst, size_t n, pid_t child) {
    /* Same disclaimer as in function above */
    for (size_t i = 0; i != n / sizeof(register_type); ++i)
        ptrace(PTRACE_POKEDATA, child, (size_t*)(dst + i), *((size_t*)(src + i)));
}

#endif

