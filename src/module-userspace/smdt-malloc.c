#include <stdlib.h>
#include <stdio.h>

int main()
{
    FILE* file = fopen("/proc/sandboxer", "w");
    fprintf(file, "1");
    fclose(file);

    malloc(512 * 1024 * 1024);

    unsigned long maxmem = 0;
    file = fopen("/proc/sandboxer", "r");
    fscanf(file, "%lu", &maxmem);
    fclose(file);

    printf("%lu\n", maxmem);
    return 0;
}
