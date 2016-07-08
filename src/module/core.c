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
#include <linux/moduleparam.h>
#include <linux/kernel.h>

#include "init.h"
#include "probes.h"
#include "slot.h"
#include "proc.h"
#include "tests/test-hashmap.h"
#include "tests/test-splay.h"

MODULE_LICENSE("GPL");

/* Insmod operation parameters: */

static int perform_hashmap_test = 0;
module_param(perform_hashmap_test, int, 0000);
static int perform_random_splay_test = 0;
module_param(perform_random_splay_test, int, 0000);
static int enable_splay_threading_test = 0;
module_param(enable_splay_threading_test, int, 0000);

/* Code: */

static int check_module_params(void) {
    if (perform_hashmap_test < 0 || perform_hashmap_test > 1) {
        printk(KERN_ERR "sandboxer: perform_hashmap_test is set to invalid value %d\n", perform_hashmap_test);
        return -EFAULT;
    }
    if (perform_random_splay_test < 0 || perform_random_splay_test > 1) { 
        printk(KERN_ERR "sandboxer: perform_random_splay_test is set to invalid value %d\n", 
               perform_random_splay_test);
        return -EFAULT;
    }
    if (enable_splay_threading_test < 0 || enable_splay_threading_test > 1) {
        printk(KERN_ERR "sandboxer: enable_splay_threading_test is set to invalid value %d\n",
               enable_splay_threading_test);
        return -EFAULT;
    }

    return 0;
}

#define SANDBOXER_PERFORM_TEST(param, func, test_param, msg) \
    if ((param) == 1) {                                      \
        if ((errno = initlib_push(func, test_param)) != 0) { \
            printk(KERN_ERR "sandboxer: " msg);              \
            return errno;                                    \
        }                                                    \
    }

static int perform_tests(void) {
    int errno;

    SANDBOXER_PERFORM_TEST(perform_hashmap_test, test_hashmap, NULL, 
                           "Tests: Hashmap tests failed\n")
    SANDBOXER_PERFORM_TEST(perform_random_splay_test, random_splay_test, NULL,
                           "Tests: Random splay tree tests failed\n")
    SANDBOXER_PERFORM_TEST(enable_splay_threading_test, init_splay_threading_test, NULL,
                           "Tests: Initialization of splay threading test failed\n")
    
    return 0;
}

static int __init sandboxer_module_init(void) {
    int errno = 0;

    printk(KERN_INFO "sandboxer: What makest thou?\n");
    printk(KERN_INFO "sandboxer: I wanna come'n sandbox'ya!\n");

    if ((errno = check_module_params()) != 0) {
        printk(KERN_ERR "sandboxer: Could not parse parameters.\n");
        goto out_error;
    }

    if ((errno = initlib_init()) != 0) {
        printk(KERN_ERR "sandboxer: Initlib initialization failed.\n");
        goto out_error;
    }

    if ((errno = perform_tests()) != 0) {
        printk(KERN_ERR "sandboxer: Tests failed.\n");
        goto out_error;
    }

    if ((errno = sandboxer_init_probes()) != 0) {
        printk(KERN_ERR "sandboxer: Failed to initialized probes subsystem.\n");
        goto out_error;
    }

    if ((errno = initlib_push(init_or_shutdown_slots, NULL)) != 0) {
        printk(KERN_ERR "sandboxer: Failed to initialize slots subsystem.\n");
        goto out_error;
    }

    if ((errno = initlib_push(init_or_shutdown_sandboxer_proc_dir, NULL)) != 0) {
        printk(KERN_ERR "sandboxer: Failed to initialize proc subsystem.\n");
        goto out_error;
    }

    return 0;
out_error:
    printk(KERN_ERR "sandboxer: Fatal error during initilization, errno is %d\n", errno);
    return errno;
}

static void __exit sandboxer_module_exit(void) {
    initlib_pop_all();    
}

module_init(sandboxer_module_init);
module_exit(sandboxer_module_exit);
