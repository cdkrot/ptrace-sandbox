#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/types.h>

typedef u8 slot_id_type;
#define NOT_SANDBOXED 255

struct sandbox_slot {
    // initially it is pointer to parent
    // but when parent dies it is set to NULL.
    struct task_struct* mentor;

    size_t num_alive; // num of processes alive in this slot
    size_t ref_cnt; // ref cnt. >= num_alive.

    size_t mem_used;
    size_t max_mem_used;
    size_t mem_limit;
    
    size_t time_used;
    size_t time_limit;

    spinlock_t lock;
};

struct slot_id_info {
    slot_id_type slot_id;
    struct llist_node llnode;
};

/*
 * Init or shutdown slot subsystem
 */
int init_or_shutdown_slots(bool is_init);

/* 
 * Returns slot id of task.
 * Or not_sandboxed.
 */
slot_id_type get_slot_of(struct task_struct*);

/*
 * Returns slot by it's id.
 * Basically, we want to decrease all direct operations with slot
 * as much as possible
 */
struct sandbox_slot* get_slot_by_id(slot_id_type);

/*
 * Creates new slot and returns it's id.
 * Returns "NOT_SANDBOXED" on failure.
 * Slot is initialized with num_alive = 0, ref_cnt = 1.
 */
slot_id_type create_new_slot(struct task_struct* mentor);

/*
 * Marks current process as related to slot.
 * ref_cnt and num_alive are increased.
 */
void attach_task_to_slot(slot_id_type);

/*
 * Detaches current process from it's slot (if has such).
 * Decreases ref_cnt and num_alive.
 * If ref_cnt reaches zero, slot is destructed.
 */
void detach_task_from_slot(void);

/*
 * Increases slot's refcnt by one.
 */
void increase_slot_refcnt(slot_id_type);

/*
 * Decreases slot's refcnt by one.
 * Slot is destructed if refcnt reaches zero.
 */
void decrease_slot_refcnt(slot_id_type);
