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

    printf("child: %d\n", s);

    return 0;
}

int main(int argc, char** argv) {
    FILE *f;
    char buf[100], buf2[100];
    int exited = 0;
    int num_slots = (1 < argc ? atoi(argv[1]) : 4);
    int slot_id, fslot_id;
    
    for (int i = 0; i < num_slots; i++) {
        if (!fork())
            return child();
    }


    for (;;) {
        f = fopen("/proc/sandboxer/notifications", "r");
        fgets(buf, 100, f);
        fclose(f);
        
        printf("%s", buf);

        if (strncmp(buf, "SLOT_CREATE", 11) == 0) {
            sscanf(buf + 12, "%d", &slot_id);
            sprintf(buf2, "/proc/sandboxer/%d/slot_id", slot_id);
            f = fopen(buf2, "r");
            fscanf(f, "%d", &fslot_id);
            fclose(f);
            printf("parent: %d\n", fslot_id);
        } else if (strncmp(buf, "SLOT_TERM", 9) == 0)
            exited++;

        if (exited == num_slots)
            break;
    }
    printf("exited: %d\n", exited);
    return 0;
}
