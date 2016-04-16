#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define USERSPACE
#include "test-splay.h"

#define ELEMS 200000

int abs(int x) {
    if (x < 0)
        return -x;
    return x;
}

int vals[PID_MAX];

void random_shuffle(int a[], int n) {
    int i, j, k;

    for (i = 0; i < n; i++) {
        j = rand() % (i + 1);
        k = a[j];
        a[j] = a[i];
        a[i] = k;
    }
}

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
    
    random_shuffle(created, ELEMS);

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

    for (int i = ELEMS / 2; i < ELEMS; i++) {
        int action = rand() % 2;
        if (action == 0) {
            if (vals[created[i]] != -1) {
                m = get_mentor_stuff(created[i]);
                if (vals[created[i]] != m->secret) {
                    printf("Failed\n");
                    return -1;
                }
                free_mentor_stuff(created[i]);
                vals[created[i]] = -1;
            } else if (get_mentor_stuff(created[i])) {
                printf("Failed\n");
                return -1;
            }
        } else {
            if (vals[created[i]] != -1) {
                m = get_mentor_stuff(created[i]);
                if (vals[created[i]] != m->secret) {
                    printf("Failed\n");
                    return -1;
                }
            } else {
                vals[created[i]] = abs(rand());
                m = create_mentor_stuff(created[i]);
                m->secret = vals[created[i]];
            }
        }
    }

    shutdown_mentor_stuff();
    printf("OK\n");

    return 0;
}
