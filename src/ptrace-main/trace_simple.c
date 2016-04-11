//  Ptrace-based code (detection of syscalls)
//  Copyright (C) 2016  Sayutin Dmitry, Vasiliy Alferov
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
