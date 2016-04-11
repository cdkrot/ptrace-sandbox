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
#include <sys/mman.h>

int main(int argc, char** argv)
{
    assert(argc > 1);
    FILE* f = fopen("/proc/sandboxer", "w");
    if (!f)
    {
        printf("Unable to open file on writing\n");
        return -1;
    }
    fprintf(f, "1");
    fclose(f);

    execve(argv[1], argv + 1, NULL);

    printf("!!!\n");

    f = fopen("/proc/sandboxer", "r");
    if (!f)
    {
        printf("Unable to open file on reading\n");
        return -1;
    }
    unsigned long maxmem;
    fscanf(f, "%lu", &maxmem);
    fclose(f);

    printf("\n\nMemory usage: %lu\n", maxmem);

    return 0;
}
