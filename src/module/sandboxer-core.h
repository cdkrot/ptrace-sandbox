#include <linux/sched.h>
#include <linux/pid.h>

struct sandbox_slot {
    size_t num_alive; /* num process alive in this slot */
    
    size_t memory_used;
    size_t max_memory_used;
    size_t memory_limit;
    
    size_t time_used;
    size_t time_limit;

    /* also should contain security rules */
};

/* num of kernel pids */
#define PID_MAX PID_MAX_LIMIT

/* num of slots, todo: make a module parametre */
#define NUM_SANDBOXING_SLOTS 128

/* contains sandboxing slot of pid, or 255, if not sandboxed */
#define NOT_SANDBOXED 255

extern u8 slot_of[PID_MAX];
extern struct sandbox_slot slots[NUM_SANDBOXING_SLOTS];

u8 create_new_slot(void); // return NOT_SANDBOXED, if no slots left.
void release_slot(u8); // call only when slot is empty.
void attach_pid_to_slot(pid_t, u8); // asserts that both pid and slot are valid.
void detach_pid_from_slot(pid_t pid);  // call when pid died and _have_been_waited_for_.
