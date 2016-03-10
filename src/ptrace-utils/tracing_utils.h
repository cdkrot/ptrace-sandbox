#ifndef PT_SNDBOX_TRACING_UTILS_H
#define PT_SNDBOX_TRACING_UTILS_H

#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>

#ifdef __x86_64
  typedef unsigned long long int register_type;
  #define REG_FMT_D  "%lld"
  #define REG_FMT_U  "%llu"
  #define REG_FMT_X  "%llx"
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

struct tracing_callbacks {
    // called when tracee recieve's signal.
    // args: pid, signal, user data ptr.
    // return: should deliver signal.
    int (*on_signal)(pid_t, int, void*);

    // called when tracee enter's or leave's syscall.
    // args: pid, entering (0) or leaving(1), user data ptr
    void (*on_syscall)(pid_t, int, void*);

    // called when tracee exits.
    // NOTE: some tracee's may silently disappear without
    // reporting exit, see ptrace(2).
    // Args: pid of just died process, it's exit code, user data ptr.
    void (*on_child_exit)(pid_t, int code, void*);

    // called when ptrace group stop event happens.
    // args: pid, user data ptr.
    void (*on_groupstop)(pid_t pid, void*);

    // called when ptrace event happens,
    // args: pid, event id (equal to one of PTRACE_EVENT_...).
    void (*on_ptrace_event)(pid_t pid, int event, void*);
};

void          extract_registers(pid_t child, struct user_regs_struct* regs);
void     extract_syscall_params(const struct user_regs_struct* regs, struct syscall_info* out);
void     extract_syscall_result(const struct user_regs_struct* regs, struct syscall_info* out);

void     tracing_loop(const struct tracing_callbacks* callbacks, void* userdata);
void     trace_me();

#endif // guard
