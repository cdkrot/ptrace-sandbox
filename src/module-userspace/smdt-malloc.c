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

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    printf("My pid is %d\n", getpid());

    FILE* file = fopen("/proc/sandboxer", "w");
    fprintf(file, "1");
    fclose(file);
    
    malloc(512 * 1024 * 1024);

    srand(time(NULL));
    usleep((rand() % 1000 + 100) * 1000);
    
    unsigned long maxmem = 0;
    file = fopen("/proc/sandboxer", "r");
    fscanf(file, "%lu", &maxmem);
    fclose(file);

    printf("%lu\n", maxmem);
    return 0;
}
