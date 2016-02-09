#ifndef PT_SNDBOX_TRACING_UTILS_H
#define PT_SNDBOX_TRACING_UTILS_H

#include <sys/types.h>
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
    register_type arg6; /* extraction of arg 6 is not supported yet */
    register_type ret;
};

void extract_syscall_params(pid_t child, struct syscall_info* out);
void extract_syscall_result(pid_t child, struct syscall_info* out);

#endif // guard
