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

struct initlib_entry {
    int (*initfunc)(int, void *);
    void *data;
    struct initlib_entry *prev;
};

static struct initlib_entry *pushed;

int initlib_init(void) {
    pushed = NULL;
    return 0;
}

int initlib_push_advanced(int (*init_func)(int, void *), void *data, const char *msg, bool unroll) {
    struct initlib_entry *entry;
    int errno = 0;

    entry = kmalloc(sizeof(struct initlib_entry), GFP_KERNEL);
    if (!entry) {
        errno = -ENOMEM;
        printk(KERN_INFO "sandboxer: failed to initialize due to lack of memory\n");
        goto out_fail;
    }

    errno = init_func(1, data);
    if (errno != 0) {
        if (msg) {
            printk(msg);
            printk(KERN_INFO "sandboxer: initialization failed with errno = %d\n", errno);
        }
        kfree(entry);
        goto out_fail;
    }

    entry->initfunc = init_func;
    entry->data = data;
    entry->prev = pushed;
    pushed = entry;
    return 0;
    
out_fail:
    if (unroll)
        initlib_pop_all();
    return errno;
}

int initlib_push(int (*init_func)(int, void *), void *data) {
    return initlib_push_advanced(init_func, data, NULL, true);
}

int initlib_push_errmsg(int (*init_func)(int, void *), void *data, const char *errmsg) {
    return initlib_push_advanced(init_func, data, errmsg, true);
}

void initlib_pop_all(void) {
    struct initlib_entry *last;

    while (pushed) {
        last = pushed;
        pushed->initfunc(0, pushed->data);
        pushed = pushed->prev;
        kfree(last);
    }
}
