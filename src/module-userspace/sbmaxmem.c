//  Sandboxer userspace part
//  Copyright (C) 2016  Vasiliy Alferov
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


#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>

int main(int argc, char** argv)
{
    assert(argc > 1);
    pid_t parent_pid = getpid();
    
    printf("parent pid is %d\n", parent_pid);

    pid_t child_pid = fork();
    if (child_pid == 0)
    {
        // child
        child_pid = getpid();

        FILE* f = fopen("/proc/sandboxer", "w");
        if (!f)
        {
            printf("Unable to open file on writing\n");
            return -1;
        }
        fprintf(f, "1");
        fclose(f);

        execve(argv[1], argv + 1, NULL);
    }
    else
    {
        // parent

        printf("child pid is %d\n", child_pid);

        FILE* f = fopen("/proc/sandboxer_info", "r");
        if (!f)
        {
            printf("Unable to open file on reading\n");
            return -1;
        }
        char c_slot_id;
        fscanf(f, "%c", &c_slot_id);
        uint8_t slot_id = c_slot_id;
        fclose(f);

        printf("\n\nSlot id: %d\n", (int)slot_id);
    }
    return 0;
}
