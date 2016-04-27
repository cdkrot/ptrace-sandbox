//  Sandboxer, kernel module sandboxing stuff
//  Copyright (C) 2016  Sayutin Dmitry, Vasiliy Alferov
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <linux/slab.h>
#include "sandboxer-slot.h"
#include "sandboxer-mentor.h"

// I dislike following sentences enormously.
#define PID_MAX PID_MAX_LIMIT
#define NUM_SANDBOXING_SLOTS 128

// it is assumed, that pid p can modify/read only slot_of[p];
slot_id_type slot_of[PID_MAX];
struct sandbox_slot slots[NUM_SANDBOXING_SLOTS];

// stack of open slots.
slot_id_type free_slots[NUM_SANDBOXING_SLOTS];
size_t p_free_slot = NUM_SANDBOXING_SLOTS;
DEFINE_SPINLOCK(stack_lock);

void release_slot(u8 slot) {
    spin_lock(&stack_lock);
    BUG_ON(p_free_slot >= NUM_SANDBOXING_SLOTS);
    free_slots[p_free_slot++] = slot;
    printk(KERN_INFO "Slot %d released, total %zu slots available now\n", (u32)slot, p_free_slot);
    spin_unlock(&stack_lock);
}

int init_or_shutdown_slots(bool is_init) {
    if (is_init) {
        slot_id_type i;
        BUG_ON(sizeof(slot_id_type) != 1); // The following line assumes it.
        memset(slot_of, NOT_SANDBOXED, PID_MAX);
        
        for (i = 0; i != NUM_SANDBOXING_SLOTS; ++i)
            free_slots[i] = NUM_SANDBOXING_SLOTS - 1 - i;
    }
    return 0;
}

slot_id_type get_slot_of(struct task_struct* process) {
    BUG_ON(process == NULL);
    BUG_ON(process->pid < 0);
    BUG_ON(process->pid >= PID_MAX);
    BUG_ON(process != current); /* important limitation */
    return slot_of[process->pid];
}

struct sandbox_slot* get_slot_by_id(slot_id_type id) {
    BUG_ON(id < 0);
    BUG_ON(id >= NUM_SANDBOXING_SLOTS);
    return &slots[id];
}

slot_id_type create_new_slot(struct task_struct* mentor) {
    slot_id_type res;
    struct mentor_stuff* ms;
    struct slot_id_info* info;

    spin_lock(&stack_lock);
    if (p_free_slot == 0) {
        spin_unlock(&stack_lock);
        return NOT_SANDBOXED;
    }
    res = free_slots[--p_free_slot];
    spin_unlock(&stack_lock);
    
    slots[res].mentor = mentor;
    slots[res].num_alive = 0;
    slots[res].ref_cnt   = 1;
    slots[res].mem_used = 0;
    slots[res].max_mem_used = 0;
    
    info = kmalloc(sizeof(struct slot_id_info), GFP_ATOMIC);
    if (!info)
        goto out_release_slot;
    
    info->slot_id = res;
    
    ms = manage_mentor_stuff(mentor->pid, MENTOR_GET_OR_CREATE);
    if (ms == NULL)
        goto out_release_info;

    spin_lock(&(ms->lock));
    llist_add(&(info->llnode), &(ms->awaited_slot_ids));
    spin_unlock(&(ms->lock));
    
    up(&(ms->counter));
    
    printk(KERN_INFO "Allocated new sandboxing slot (%u; mentor is %d)\n", (u32)res, mentor->pid);

    return res;
out_release_info:
    kfree(info);
out_release_slot:
    release_slot(res);
    return NOT_SANDBOXED;
}

void attach_task_to_slot(slot_id_type slot) {
    BUG_ON(current->pid >= PID_MAX);
    BUG_ON(current->pid < 0);
    BUG_ON(slot >= NUM_SANDBOXING_SLOTS);
    BUG_ON(slot_of[current->pid] != NOT_SANDBOXED);
    
    /* we want to recieve all events related to syscall enter.
       so we set this flag.
       Setting tif_seccomp results in panic()... =( */
    // TODO: Is this safe?
    set_thread_flag(TIF_SYSCALL_AUDIT);
    
    slot_of[current->pid] = slot;
    
    slots[slot].num_alive += 1;
    slots[slot].ref_cnt += 1;
    printk(KERN_INFO "Attached pid (%d) to slot (%u)\n", current->pid, (u32)slot);
}

void detach_task_from_slot(void) {
    BUG_ON(current->pid >= PID_MAX);
    BUG_ON(current->pid < 0);

    if (slot_of[current->pid] != NOT_SANDBOXED) {
        printk("Dies, %zu, %zu.\n", slots[slot_of[current->pid]].num_alive, slots[slot_of[current->pid]].ref_cnt);
        BUG_ON(slots[slot_of[current->pid]].num_alive == 0);
        slots[slot_of[current->pid]].num_alive -= 1;

        decrease_slot_refcnt(slot_of[current->pid]);
        
        slot_of[current->pid] = NOT_SANDBOXED;
    }
}

void increase_slot_refcnt(slot_id_type id) {
    slots[id].ref_cnt += 1;
}

void decrease_slot_refcnt(slot_id_type id) {
    BUG_ON(slots[id].ref_cnt == 0);
    slots[id].ref_cnt -= 1;
    
    if (slots[id].ref_cnt == 0)
        release_slot(id);
}
