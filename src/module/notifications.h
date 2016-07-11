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

#ifndef NOTIFICATIONS_H_
#define NOTIFICATIONS_H_

#include "slot.h"

#include <linux/pid.h>

struct slot_create_notification {
    int slot_id;
};

struct slot_stop_notification {
    int slot_id;
    char *reason;
};

struct slot_term_notification {
    int slot_id;
};

struct sec_viol_notification {
    int slot_id;
};

struct mem_lim_notification {
    int slot_id;
};

enum notification_type_t {
    SLOT_CREATE,
    SLOT_STOP,
    SLOT_TERM,
    SEC_VIOL,
    MEM_LIM
};

struct notification {
    enum notification_type_t type;
    union {
        struct slot_create_notification    slot_create;
        struct slot_stop_notification      slot_stop;
        struct slot_term_notification      slot_term;
        struct sec_viol_notification       sec_viol;
        struct mem_lim_notification        mem_lim;
    } data;
};

/*
 * Those functions exists to control memory waste by notification queues.
 * After slot creation increase_mentor_refcnt(mentor) must be called.
 * After slot death (_all_ processes of slot have exited and no notifications will be received for that slot
 * so long) decrease_mentor_refcnt(mentor) must be called.
 * Note that both functions does not report any notifications.
 *
 * Return value of increase_mentor_refcnt is errno (-ENOMEM) if no memory found for creating notification
 * queue. 
 **/
int increase_mentor_refcnt(struct pid *pid);
void decrease_mentor_refcnt(struct pid *pid);

/*
 * Does that need an explanation?
 **/
void send_notification(struct pid *pid, struct notification n);

/*
 * This returns first notification in the queue. The process will sleep until the notification appear.
 **/
int read_notification(struct pid *pid, struct notification *n);

/*
 * Folowing functions just send notifications of all types
 **/

static inline void send_slot_create_notification(struct sandbox_slot *s) {
    struct notification n;
    struct slot_create_notification data;
    
    n.type = SLOT_CREATE;
    data.slot_id = s->slot_id;

    n.data.slot_create = data;

    send_notification(s->mentor, n);
}

static inline void send_slot_stop_notification(struct sandbox_slot *s, char *reason) {
    struct notification n;
    struct slot_stop_notification data;

    n.type = SLOT_STOP;
    data.slot_id = s->slot_id;
    data.reason = reason;

    n.data.slot_stop = data;

    send_notification(s->mentor, n);
}

static inline void send_slot_term_notification(struct sandbox_slot *s) {
    struct notification n;
    struct slot_term_notification data;

    n.type = SLOT_TERM;
    data.slot_id = s->slot_id;

    n.data.slot_term = data;

    send_notification(s->mentor, n);
}

static inline void send_sec_viol_notification(struct sandbox_slot *s) {
    struct notification n;
    struct sec_viol_notification data;

    n.type = SEC_VIOL;
    data.slot_id = s->slot_id;

    n.data.sec_viol = data;

    send_notification(s->mentor, n);
}

static inline void send_mem_lim_notification(struct sandbox_slot *s) {
    struct notification n;
    struct mem_lim_notification data;

    n.type = MEM_LIM;
    data.slot_id = s->slot_id;

    n.data.mem_lim = data;

    send_notification(s->mentor, n);
}

#endif //NOTIFICATIONS_H_
