#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <unistd.h>

int child() {
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

    usleep((rand() % 1000 + 100) * 10);

    return 0;
}

int main(int argc, char** argv) {
    srand(time(0));
    FILE *f;
    char buf[100];
    int exited = 0;
    int num_slots = (1 < argc ? atoi(argv[1]) : 4);
    
    for (int i = 0; i < num_slots; i++) {
        if (!fork())
            return child();
    }


    for (;;) {
        f = fopen("/proc/sandboxer/notifications", "r");
        fgets(buf, 100, f);
        fclose(f);
        
        printf("%s", buf);

        if (strncmp(buf, "SLOT_TERM", 9) == 0)
            exited++;

        if (exited == num_slots)
            break;
    }
    printf("exited: %d\n", exited);
    return 0;
}
