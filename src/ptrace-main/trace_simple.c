#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "die.h"

int main(int argc, char** argv) {
    if (argc < 2)
        die(1, "Not enough args: %d wanted, %d supplied\n", 2, argc - 1);

    pid_t res = fork();
    if (res < 0)
        die(1, "Fork failed\n");
    else if (res == 0) {
        // child
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        kill(getpid(), SIGSTOP);
        execve(argv[1], argv + 1, NULL);
    } else {
        for (;;) {
            int status;
            pid_t child = waitpid(-1, &status, __WALL);
            
            if (errno == ECHILD) {
                errno = 0;
                break;
            }

            if (errno != 0)
                die(1, "Encountered unexpected error %d (%s)\n", errno, strerror(errno));
            
            ptrace(PTRACE_SYSCALL, child, NULL, NULL);
        }
    }
}
