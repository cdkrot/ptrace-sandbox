//  Sandboxer, kernel module sandboxing stuff
//  Copyright (C) 2016  Sayutin Dmitry, Alferov Vasiliy
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

/*
 * This module is written to control memory waste during module execution. In debug mode it prints useful
 * statistics. Please, use debug mode only when necessary. 
 **/

#ifndef SANDBOXER_MEMCONTROL_H_
#define SANDBOXER_MEMCONTROL_H_

#include <linux/types.h>
#include <linux/slab.h>

#define SBMT_NUM 7 /* Please increase that number every time you add a target */

enum sandboxer_malloc_target {
    SBMT_SLOT,                   // [slot.c]              allocate struct sandbox_slot
    SBMT_HASHMAP,                // [hashmap.c]           allocations for hashmaps
    SBMT_NOTIFICATIONS_STRUCT,   // [notifications.c]     allocate struct notification_queue
    SBMT_NOTIFICATION_QUEUE,     // [notifications.c]     allocations for notification queues
    SBMT_NOTIFICATION,           // [proc.c]              allocate notification
    SBMT_PROPERTY,               // [proc.c]              allocate struct property
    SBMT_PROPERTY_FILE           // [proc.c]              allocate struct property_file
};

int init_or_shutdown_memcontrol(int initlib_mode, __attribute__((unused)) void *ignored);

/*
 * You should use these kmalloc replacements everywhere in sandboxer. This does not affect tests, though.
 **/
void *sb_kmalloc(size_t size, gfp_t flags, enum sandboxer_malloc_target sbmt);
void sb_kfree(const void *v);

#endif //SANDBOXER_MEMCONTROL_H_
