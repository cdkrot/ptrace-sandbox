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

#ifndef SANDBOXER_MENTOR_H_
#define SANDBOXER_MENTOR_H_

#include <linux/types.h>
#include <linux/pid.h>
#include <linux/llist.h>

#include "sandboxer-core.h"

struct mentor_stuff {
    pid_t pid;
    struct llist_head awaited_slot_ids;
};

int init_mentor_stuff(void);
void shutdown_mentor_stuff(void);

struct mentor_stuff *get_mentor_stuff(pid_t pid);
struct mentor_stuff *create_mentor_stuff(pid_t pid);

// All inner information in struct mentor_stuff should be already cleaned.
void free_mentor_stuff(pid_t pid);

#endif //SANDBOXER_MENTOR_H_
