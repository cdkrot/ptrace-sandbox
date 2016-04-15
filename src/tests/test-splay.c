#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define USERSPACE
#include "test-splay.h"

#define ELEMS 100000

int abs(int x) {
    if (x < 0)
        return -x;
    return x;
}

int vals[PID_MAX];

int main() {
    int i, j;
    int created[ELEMS];
    struct mentor_stuff *m;

    int random_seed = time(0);
    //fprintf(stderr, "random_seed=%d\n", random_seed);
    srand(random_seed);
    for (i = 0; i < PID_MAX; i++)
        vals[i] = -1;

    printf("Performing splay tree test... ");
    fflush(stdout);
    init_mentor_stuff();
    
    for (i = 0; i < ELEMS; i++) {
        j = rand() % PID_MAX;
        created[i] = j;
        if (vals[j] == -1) {
            vals[j] = abs(rand());
            m = create_mentor_stuff(j);
            m->secret = vals[j];
        }
    }
    
    for (i = 0; i < ELEMS / 2; i++) {
        if (vals[created[i]] != -1) {
            m = get_mentor_stuff(created[i]);
            if (vals[created[i]] != m->secret) {
                printf("Failed\n");
                return -1;
            }
            m = NULL;
            free_mentor_stuff(created[i]);
            vals[created[i]] = -1;
        }
    }

    for (int i = ELEMS / 2; i < ELEMS; i++) {
        if (vals[created[i]] != -1) {
            m = get_mentor_stuff(created[i]);
            if (vals[created[i]] != m->secret) {
                printf("Failed\n");
                return -1;
            }
        }
    }

    shutdown_mentor_stuff();
    printf("OK\n");

    return 0;
}
