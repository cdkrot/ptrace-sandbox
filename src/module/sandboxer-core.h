//  Sandboxer, kernel module sandboxing stuff
//  Copyright (C) 2016  Vasiliy Alferov, Sayutin Dmitry
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

#ifndef SANDBOXER_CORE_H_
#define SANDBOXER_CORE_H_

#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/types.h>

struct sandbox_slot {
    pid_t mentor;

    size_t num_alive; /* num process alive in this slot */
    size_t ref_cnt; /* reference counter, >= num_alive */
    
    size_t memory_used;
    size_t max_memory_used;
    size_t memory_limit;
    
    size_t time_used;
    size_t time_limit;

//    DEFINE_SPINLOCK(slot_lock);
    /* also should contain security rules */
};

struct slot_id_info {
    u8 slot_id;
    struct llist_node llnode;
};

/* num of kernel pids */
#define PID_MAX PID_MAX_LIMIT

/* num of slots, todo: make a module parametre */
#define NUM_SANDBOXING_SLOTS 128

/* contains sandboxing slot of pid, or 255, if not sandboxed */
#define NOT_SANDBOXED 255

extern u8 slot_of[PID_MAX];
extern struct sandbox_slot slots[NUM_SANDBOXING_SLOTS];
extern struct llist_head awaited_slot_ids[PID_MAX];
extern struct slot_id_info allocated_slot_ids[PID_MAX];

u8 create_new_slot(pid_t mentor); // return NOT_SANDBOXED, if no slots left.
void release_slot(u8); // call only when slot is empty.
void attach_pid_to_slot(pid_t, u8); // asserts that both pid and slot are valid.
void detach_pid_from_slot(pid_t pid);  // call when pid died and _have_been_waited_for_.

#endif //SANDBOXER_CORE_H_
