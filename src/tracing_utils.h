#ifndef PT_SNDBOX_TRACING_UTILS_H
#define PT_SNDBOX_TRACING_UTILS_H

#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>

#ifdef __x86_64
  typedef unsigned long long int register_type;
  #define REG_FORMAT "%lld"
#else
  #error "Sorry, your platform is not supported yet."
#endif

struct syscall_info {
    register_type id;
    register_type arg1;
    register_type arg2;
    register_type arg3;
    register_type arg4;
    register_type arg5;
    register_type arg6;
    register_type ret;
    register_type err;
};

void             memcpy_to_proc(void* src, void* dst, size_t n, pid_t child);
void           memcpy_from_proc(void* src, void* dst, size_t n, pid_t child);

void              dump_regisers(const struct user_regs_struct* regs);
void*         get_stack_pointer(const struct user_regs_struct* regs);
void          extract_registers(pid_t child, struct user_regs_struct* regs);
void     extract_syscall_params(const struct user_regs_struct* regs, struct syscall_info* out);
void     extract_syscall_result(pid_t child, const struct user_regs_struct* regs, struct syscall_info* out);


#endif // guard
