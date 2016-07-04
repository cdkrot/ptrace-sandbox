#ifndef SANDBOXER_SLOT_H_
#define SANDBOXER_SLOT_H_

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/types.h>

struct sandbox_slot {
    struct pid* mentor; /* pointer to mentor, never null */

    size_t num_alive; /* num alive pids inside */
    size_t ref_cnt; /* extra ref counter */

    size_t slot_id; /* for purpose of userspace IO */

    size_t mem_used;
    size_t mem_max_usage;
    size_t mem_limit;

    size_t time_usage;
    size_t time_limit;

    spinlock_t lock;
//  struct proc_dir_entry slotid_dir; /* handle to /proc entry  via proc.{c,h}
};

/**
  * Returns slot of current task, or NULL
  */
struct sandbox_slot* get_slot_of(void);

/**
  * Creates new slot, or more precisely: 
  *
  * Increases ref cnt of mentor, so it won't die
  * allocates new slot
  * Attaches current to this slot
  *
  * Returns NULL on failure.
  */
struct sandbox_slot* create_slot(void);

/**
  * Call when task dies. Releases slot, if necessary.
  * You must not not own slot lock, while calling this func.
  *
  * (If sanboxed)  Detaches task from slot
  * (If slot died) Deletes slot if necessary
  * (If slot died) Decreases ref cnt of mentor
  */
void release_slot(void);

/**
  * Call when your extra reference is no longer required. Releases slot, if necessary.
  * You are expected to not own slot lock, while calling this func.
  *
  * (always) Decreases ref counter.
  * (If slot empty) Deletes slot if necessary.
  * (If slot empty) Decreases ref cnt of mentor.
  *
  * TODO: Is slot extra ref mechanic really used somewhere?
  */
void release_slot_ref(struct sandbox_slot* slot);

/**
  * Call when you need to get extra slot reference.
  * You are expected to _hold_ the slot lock.
  *
  * TODO: Is slot extra ref mechanic really used somewhere?
  */
struct sandbox_slot* get_slot_ref(struct sandbox_slot* slot);

/**
  * Use with initlib
  */
int init_or_shutdown_slots(int is_init, void* ign);

#endif
