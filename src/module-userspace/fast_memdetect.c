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
        printf("Unable to open file\n");
        return -1;
    }
    fprintf(f, "1");
    fclose(f);

    execve(argv[1], argv + 1, NULL);

    mmap(0, 0, 0, 0, 0, 0);

    return 0;
}
