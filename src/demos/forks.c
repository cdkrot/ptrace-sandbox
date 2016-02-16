#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void recursive(int cnt) {
    if (cnt != 0) {
        fork();
        recursive(cnt - 1);
    }
}

int main(int argc, char** argv) {
    assert(argc == 2);
    recursive(atoi(argv[1]));
    printf("Done\n");
}
