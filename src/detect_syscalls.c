#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>   /* For SYS_write etc */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __x86_64
  typedef long long int register_type;
  #define REG_FORMAT "%lld"
#else
  typedef long int      register_type;
  #define REG_FORMAT "%ld"
#endif

struct syscall_info {
    register_type id;
    register_type arg1;
    register_type arg2;
    register_type arg3;
    register_type arg4;
    register_type arg5; /* extraction of arg 6 is not supported yet */
};

struct syscall_info extract_syscall_info(pid_t child) {
    struct user_regs_struct registers;
    ptrace(PTRACE_GETREGS, child, 0, &registers);

    struct syscall_info result;
    #ifdef __x86_64
    result.id   = registers.orig_rax;
    result.arg1 = registers.rdi;
    result.arg2 = registers.rsi;
    result.arg3 = registers.rdx;
    result.arg4 = registers.r10;
    result.arg5 = registers.r8;
    #else
    #error "Other platforms unsupported yet"
    #endif
    
    return result;
}

const char* get_syscall_name(register_type id) {
    switch (id) {
        case SYS_write:
            return "write";
        case SYS_read:
            return "read";
        case SYS_open:
            return "open";
        case SYS_close:
            return "close";
        case SYS_execve:
            return "execve";
        case SYS_exit:
            return "exit";
        case SYS_mmap:
            return "mmap";
        case SYS_munmap:
            return "munmap";
        case SYS_mprotect:
            return "mprotect";
        case SYS_lseek:
            return "lseek";
        case SYS_access:
            return "access";
        case SYS_lstat:
            return "lstat";
        case SYS_brk:
            return "brk";
        case SYS_fstat:
            return "fstat";
        case SYS_exit_group:
            return "exit_group";
        default:
            return "unknown";
    }
}

void get_syscall_descr(char* buf, size_t sz, register_type id) {
    snprintf(buf, sz, "%s (" REG_FORMAT ")", get_syscall_name(id), id);
}
    
int main(int argc, char** argv) {
    if (argc < 2)
        return -1;

    pid_t child = fork();

    if (child == -1)
        return -1;
    
    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execve(argv[1], argv + 1, NULL);
    } else {
        int status;
        bool insyscall = false;
        struct syscall_info inf;
        char buf[100];
        
        wait(&status);
        bool firsttime = 1;
        
        while(!WIFEXITED(status)) {
            if (errno) {
                fprintf(stderr, "error, %s\n", strerror(errno));
                errno = 0;
            }
            if (WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | (firsttime ? 0 : 0x80))) {
                if (insyscall == 0) {
                    // syscall start
                    inf = extract_syscall_info(child);
                    get_syscall_descr(buf, sizeof(buf), inf.id);
                    fprintf(stderr, "Sys call %s, params "
                            REG_FORMAT " " REG_FORMAT " " REG_FORMAT " " REG_FORMAT "\n",
                            buf, inf.arg1, inf.arg2, inf.arg3, inf.arg4);
                } else {
                    //syscall return
                    inf = extract_syscall_info(child);
                    fprintf(stderr, "Sys call %s, return " REG_FORMAT "\n", buf, inf.id);
                    
                    if (firsttime) {
                        // first registered syscall (execve),
                        // enable sandboxing.
                        firsttime = false;
                        ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_EXITKILL | PTRACE_O_TRACESYSGOOD);
                    }
                }
                insyscall = !insyscall;
            } else {
                fprintf(stderr, "Unknown tracing event\n");
            }
            
            ptrace(PTRACE_SYSCALL, child, NULL, NULL); // continue tracing.
            wait(&status);
        }
        fprintf(stderr, "Program exited with %d\n", WEXITSTATUS(status));
    }
    return 0;
}
