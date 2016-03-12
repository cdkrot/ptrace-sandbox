#include <stdlib.h>
#include <sys/mman.h>

const size_t num_mmaps = 1000 * 1000;
const size_t mmap_sz   = 1000;

int main() {
    for (size_t i = 0; i != num_mmaps; ++i)
        munmap(mmap(NULL, mmap_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0), mmap_sz);
    return 0;
}
