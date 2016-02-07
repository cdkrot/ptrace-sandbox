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
        bool insyscall = false;
        char buf[100];
        
        wait(&status);
        bool firsttime = 1;
        
        while(!WIFEXITED(status)) {
            if (errno) {
                fprintf(stderr, "error, %s\n", strerror(errno));
                errno = 0;
            }
            if (WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | (firsttime ? 0 : 0x80))) {
                struct user_regs_struct registers;
                ptrace(PTRACE_GETREGS, child, 0, &registers);
                
                if (insyscall == 0) {
                    // syscall start
                    get_syscall_descr(buf, sizeof(buf), registers.ORIG_EAX);
                    fprintf(stderr, "Sys call %s, params " REG_FORMAT " " REG_FORMAT " " REG_FORMAT "\n",
                            buf, registers.EBX, registers.ECX, registers.EDX);
                } else {
                    //syscall return
                    fprintf(stderr, "Sys call %s, return " REG_FORMAT "\n", buf, registers.EAX);
                    
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
