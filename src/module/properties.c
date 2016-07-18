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

#include <linux/seq_file.h>
#include "properties.h"
#include "proc.h"

/* 
 * Here is an example of callback.
 **/
static int slot_id_callback(struct seq_file *s, size_t slot_id) {
    seq_printf(s, "%lu\n", slot_id);
    return 0;
}

#define ADD_PROPERTY(name, cb)                         \
    if ((errno = add_slot_property(name, cb)) != 0) {  \
        return errno;                                  \
    }

int init_or_shutdown_properties(int initlib_mode, __attribute__((unused)) void *ignored) {
    int errno = 0;

    if (initlib_mode) {
        ADD_PROPERTY("slot_id", slot_id_callback); 

        return 0;
    } else {
        return 0;
    }
};
