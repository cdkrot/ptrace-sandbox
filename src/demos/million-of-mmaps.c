#include <stdlib.h>
#include <sys/mman.h>

int main() {
    for (int i = 0; i < 1000 * 1000; i++)
        munmap(mmap(0, i + 1, PROT_NONE, MAP_PRIVATE, 0, 0), i + 1);
    return 0;
}
