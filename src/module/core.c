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

#include <linux/module.h>
#include <linux/kernel.h>

#include "init.h"
#include "probes.h"

MODULE_LICENSE("GPL");

static int dummy_init_func(int init, void *_data) {
    long data;

    data = *((long*)_data);
    if (init) {
        printk(KERN_INFO "dummy_init_func: init %ld\n", data);
        return 0;
    } else {
        printk(KERN_INFO "dummy_init_func: exit %ld\n", data);
        return 0;
    }
}

static int failing_init_func(int init, void *_data) {
    if (init)
        return -EFAULT;
    return 0;
}

long twos[11];

static int __init sandboxer_module_init(void) {
    int errno = 0, i;

    printk(KERN_INFO "sandboxer: What makest thou?\n");

    if ((errno = initlib_init())) {
        printk(KERN_ERR "sandboxer: Initlib initialization failed with errno %d. Shutting down", errno);
        return errno;
    }

    for (i = 0; i < 11; i++) {
        twos[i] = 1 << i;
        initlib_push(dummy_init_func, twos + i);
    }

    if ((errno = sandboxer_init_probes()) != 0) {
        printk(KERN_ERR "sandboxer: Failed to initialized probes subsystem. Errno = %d\n", errno);
        return errno;
    }
    //if ((errno = initlib_push_errmsg(failing_init_func, NULL, KERN_ERR "sandboxer: as waited")))
        //return errno;

    return 0;
}

static void __exit sandboxer_module_exit(void) {
    initlib_pop_all();    
}

module_init(sandboxer_module_init);
module_exit(sandboxer_module_exit);
