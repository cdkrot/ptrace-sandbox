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

#include <linux/slab.h>
#include <linux/errno.h>

#include "init.h"

struct forward_list_head {
    struct forward_list_head *next;
};

struct initlib_entry {
    int (*initfunc)(int, void *);
    void *data;
    struct forward_list_head node;
};

static struct forward_list_head *pushed;

int initlib_init(void) {
    pushed = NULL;
    return 0;
}

static int initlib_do_push(int (*init_func)(int, void *), void *data, bool print, const char *msg) {
    struct initlib_entry *entry;
    int errno = 0;

    entry = kmalloc(sizeof(struct initlib_entry), GFP_KERNEL);
    if (!entry) {
        errno = -ENOMEM;
        goto out;
    }

    errno = init_func(1, data);
    if (errno != 0) {
        if (print) {
            printk(msg);
            printk(KERN_INFO "sandboxer initlib: errno = %d\n", errno);
        }
        initlib_pop_all();
        goto out_free_entry;
    }

    entry->initfunc = init_func;
    entry->data = data;
    entry->node.next = pushed;
    pushed = &entry->node;
    goto out;

out_free_entry:
    kfree(entry);
out:
    return errno;
}

int initlib_push(int (*init_func)(int, void *), void *data) {
    return initlib_do_push(init_func, data, false, NULL); 
}

int initlib_push_errmsg(int (*init_func)(int, void *), void *data, const char *errmsg) {
    return initlib_do_push(init_func, data, true, errmsg);
}

void initlib_pop_all(void) {
    struct initlib_entry *entry;

    while (pushed) {
        entry = container_of(pushed, struct initlib_entry, node);
        entry->initfunc(0, entry->data);
        pushed = entry->node.next;
        kfree(entry);
    }
}
