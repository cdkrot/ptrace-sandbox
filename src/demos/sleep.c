#include <time.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    printf("Before sleep\n");
    usleep(500);
    printf("After sleep\n");
}
