#ifndef SANDBOXER_MENTOR_H_
#define SANDBOXER_MENTOR_H_

#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

struct mentor_stuff {
    pid_t pid;
    int secret;
};

int init_mentor_stuff(void);
void shutdown_mentor_stuff(void);

struct mentor_stuff *get_mentor_stuff(pid_t pid);
struct mentor_stuff *create_mentor_stuff(pid_t pid);
void free_mentor_stuff(pid_t pid);

#ifndef USERSPACE
// Here comes rewritten kernel stuff

#define BUG_ON(x) assert(!(x))
#define GFP_KERNEL 0

void* kmalloc(size_t sz, __attribute__((unused)) const int t) {
    return malloc(sz);
}

void kfree(void* v) {
    free(v);
}

#else

#define PID_MAX (4 * 1024 * 1024)

#endif //USERSPACE

#endif //SANDBOXER_MENTOR_H_
