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
