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

#include "test-hashmap.h"
#include "../hashmap.h"
#include <linux/random.h>
#include <linux/slab.h>

static u32 get_random(u32* seed, u32 min, u32 max) {
    *seed = next_pseudo_random32(*seed);
    return min + (*seed) % (max - min + 1);
}

static size_t hash_trivial(const struct hashmap* hmp, const void* obj, size_t tryid) {
    size_t thehash = (size_t)obj;
    thehash %= hmp->size;
    thehash += tryid * tryid;
    thehash %= hmp->size;

    return thehash;
}

static bool equal_trivial(const void* obj1, const void* obj2) {
    return obj1 == obj2;
}

int test_hashmap(int initlib_mode, void* ignored) {
    int i;
    int j;
    u32 seed = 2016;

    if (initlib_mode != 1)
        return 0;

    printk(KERN_INFO "Sandboxer: testing hashmap");
    for (i = 0; i != 5; ++i) {
#define HMPTEST_EXPECT(cond, errmsg)            \
        if (!(cond)) {                          \
            hashmap_free(&hmp);                 \
            printk(KERN_INFO errmsg);           \
            goto test_failed;                   \
        }

#define TEST_KRANGE 4500
#define TEST_VRANGE 200
#define TEST_ITERS 2000

#define CHECK_RANDOM_VAL()                                                       \
        do {                                                                     \
            size_t key = get_random(&seed, 1, TEST_KRANGE - 1);                  \
            size_t val = (size_t)hashmap_get(&hmp, (void*)key);                  \
            HMPTEST_EXPECT(realvals[key] == val, "Hashmap returns wrong value"); \
        } while (0)

        size_t* realvals;
        struct hashmap hmp;

        hmp.hash = hash_trivial;
        hmp.equal = equal_trivial;
        hmp.tolerance = 30;
        HMPTEST_EXPECT(init_hashmap(&hmp, get_random(&seed, 5000, 10000)) == 0, "failed hashmap creation");

        HMPTEST_EXPECT((realvals = kmalloc(sizeof(*realvals) * TEST_KRANGE, GFP_KERNEL)) != NULL, "failed temp memory allocation");

        for (j = 0; j != TEST_KRANGE; ++j)
            realvals[j] = 0;

        // TESTING PHASES:
        // (a) Create-modify-check phase, create or modify nodes, check values.
        // (b) Destroy-check phase, delete nodes, check values.
        // Each phase repeated 4 times.
        for (j = 0; j != 8 * TEST_ITERS; ++j)
            if (j % 2)
                CHECK_RANDOM_VAL();
            else {
                int phase          = (j / TEST_ITERS) % 2;
                size_t key         = get_random(&seed, 1, TEST_KRANGE - 1);
                size_t newval      = get_random(&seed, (phase == 0 ? 1 : 0), (phase == 0 ? TEST_VRANGE - 1 : 0));
                void* oldval_hmp;
                int ret            = hashmap_set(&hmp, (void*)key, (void*)newval, &oldval_hmp);
                HMPTEST_EXPECT(ret == 0, "Hashmap operation errored");
                HMPTEST_EXPECT((size_t)oldval_hmp == realvals[key], "Hashmap returns wrong data");
                realvals[key] = newval;
            }
    }
    printk(KERN_INFO "Hashmap tests succeeded");
    return 0;
test_failed:
    printk(KERN_INFO "Failed hashmap tests");
    return -1;
}
