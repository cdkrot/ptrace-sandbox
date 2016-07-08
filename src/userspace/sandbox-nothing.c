#include <stdio.h>
#include <assert.h>

int main() {
    FILE *f;
    int s;

    f = fopen("/proc/sandboxer/sandbox_me", "w");
    assert(f);
    fputs("1", f);
    fclose(f);

    f = fopen("/proc/sandboxer/sandbox_me", "r");
    assert(f);
    assert(fscanf(f, "%d", &s) == 1);
    fclose(f);

    printf("%d\n", s);
    return 0;
}
