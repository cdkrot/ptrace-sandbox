#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>   /* For SYS_write etc */

#include <stdio.h>

int main(int argc, const char** argv) {
    if (argc < 2)
        return -1;
    
    pid_t child = fork();

    if (child == -1)
        return -1;
    
    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(argv[1], argv[1], NULL);
    } else {
        int status;
        int insyscall = 0;
        while(1) {
            wait(&status);
            if(WIFEXITED(status))
                break;

            struct user_regs_struct registers;
            ptrace(PTRACE_GETREGS, child, 0, &registers);
            
            if (insyscall == 0) {// syscall start
                #ifndef __x86_64__
                printf("Sys call %ld, params %ld %ld %ld\n", registers.orig_eax, registers.ebx, registers.ecx, registers.edx);
                #else
                printf("Sys call %lld, params %lld %lld %lld\n", registers.orig_rax, registers.rbx, registers.rcx, registers.rdx);
                #endif
            } else {
                //syscall return
                #ifndef __x86_64__
                printf("Sys call %ld, return %ld\n", registers.orig_eax, registers.eax);
                #else
                printf("Sys call %lld, return %lld\n", registers.orig_rax, registers.rax);
                #endif
            }
            insyscall = 1 - insyscall;
            
            ptrace(PTRACE_SYSCALL, child, NULL, NULL); // continue tracing.
        }
    }
    return 0;
}
