//  Sandboxer, kernel module sandboxing stuff
//  Copyright (C) 2016  Alferov Vasiliy, Sayutin Dmitry
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

#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/slab.h>
#include <linux/thread_info.h>
#include "hashmap.h"
#include "notifications.h"
#include "slot.h"

static struct hashmap hmp;
static int hashmap_size_hint = 500;
static int hashmap_tolerance =  50;

/**
  * This implementation uses auto increasing slot uid.
  * 2^31 values is more than enough for years of work.
  */
static atomic_t slot_uid_gen = ATOMIC_INIT(0);

module_param(hashmap_size_hint, int, 0);
module_param(hashmap_tolerance, int, 0);

int init_or_shutdown_slots(int is_init, void* ign) {
    if (is_init) {
        if (hashmap_size_hint <= 0 || hashmap_tolerance <= 0)
            return -EINVAL;
        hmp.tolerance = hashmap_tolerance;
        hmp.equal = hashmap_compare_simple;

        // NOTE: if you decide to change this hasher
        // to something more advanced, than implement new function,
        // instead of changing behaviour of existing.
        hmp.hash  = hashmap_hash_simple;

        return init_hashmap(&hmp, hashmap_size_hint);
    } else {
        hashmap_free(&hmp);
        return 0;
    }
}

struct sandbox_slot* get_slot_of(void) {
    return hashmap_get(&hmp, task_pid(current));
}

struct sandbox_slot* create_slot(void) {
    struct sandbox_slot* res;

    if (get_slot_of() != NULL)
        return NULL;

    res = kmalloc(sizeof(struct sandbox_slot), GFP_KERNEL);
    if (!res)
        return NULL;

    if (hashmap_set(&hmp, task_pid(current), res, NULL)) {
        kfree(res);
        return NULL;
    }

    if (increase_mentor_refcnt(task_pid(current->real_parent))) {
        hashmap_set(&hmp, task_pid(current), NULL, NULL);
        kfree(res);
        return NULL;
    }

    res->mentor = get_pid(task_pid(current->real_parent));

    res->num_alive = 1;
    res->ref_cnt   = 0;
    res->slot_id   = atomic_inc_return(&slot_uid_gen);

    res->mem_used = 0;
    res->mem_max_usage = 0;
    res->mem_limit = 0;

    res->time_usage = 0;
    res->time_limit = 0;

    // pretend, that userspace syscall audition feature enabled.
    // this way we can trace all syscall events.
    set_thread_flag(TIF_SYSCALL_AUDIT);

    spin_lock_init(&res->lock);

    send_slot_create_notification(res);

    return res;
}

void release_slot(void) {
    struct sandbox_slot* pslot = get_slot_of();

    if (pslot) {
        bool del;
        unsigned long flags;
        spin_lock_irqsave(&pslot->lock, flags);

        pslot->num_alive -= 1;
        BUG_ON(hashmap_set(&hmp, task_pid(current), NULL, NULL) != 0);
        del = (pslot->num_alive == 0 && pslot->ref_cnt == 0);

        spin_unlock_irqrestore(&pslot->lock, flags);

        if (del) {
            send_slot_term_notification(pslot);
            decrease_mentor_refcnt(pslot->mentor);

            put_pid(pslot->mentor);
            kfree(pslot);
        }
    }
}

void release_slot_ref(struct sandbox_slot* pslot) {
    unsigned long flags;
    bool del;

    BUG_ON(pslot == NULL);
    spin_lock_irqsave(&pslot->lock, flags);

    pslot->ref_cnt -= 1;
    del = (pslot->ref_cnt == 0 && pslot->num_alive == 0);

    spin_unlock_irqrestore(&pslot->lock, flags);
    if (del) {
        send_slot_term_notification(pslot);
        decrease_mentor_refcnt(pslot->mentor);

        put_pid(pslot->mentor);
        kfree(pslot);
    }
}

struct sandbox_slot* get_slot_ref(struct sandbox_slot* pslot) {
    unsigned long flags;
    BUG_ON(pslot == NULL);

    spin_lock_irqsave(&pslot->lock, flags);
    pslot->ref_cnt += 1;
    spin_unlock_irqrestore(&pslot->lock, flags);

    return pslot;
}
