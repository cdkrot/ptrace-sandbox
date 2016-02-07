#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>   /* For SYS_write etc */

#include <stdio.h>

#ifdef __x86_64
  typedef long long int register_type;
  #define REG_FORMAT "%lld"
  #define ORIG_EAX orig_rax
  #define EAX      rax
  #define EBX      rbx
  #define ECX      rcx
  #define EDX      rdx
#else
  typedef long int      register_type;
  #define REG_FORMAT "%ld"
  #define ORIG_EAX orig_eax
  #define EAX      eax
  #define EBX      ebx
  #define ECX      ecx
  #define EDX      edx
#endif

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
        int insyscall = 0;
        char buf[100];
        while(1) {
            wait(&status);
            if(WIFEXITED(status))
                break;

            struct user_regs_struct registers;
            ptrace(PTRACE_GETREGS, child, 0, &registers);

            get_syscall_descr(buf, sizeof(buf), registers.ORIG_EAX);
            
            if (insyscall == 0) {
                // syscall start                 
                printf("Sys call %s, params " REG_FORMAT " " REG_FORMAT " " REG_FORMAT "\n",
                       buf, registers.EBX, registers.ECX, registers.EDX);
            } else {
                //syscall return
                printf("Sys call %s, return " REG_FORMAT "\n", buf, registers.EAX);
            }
            insyscall = 1 - insyscall;
            
            ptrace(PTRACE_SYSCALL, child, NULL, NULL); // continue tracing.
        }
    }
    return 0;
}
