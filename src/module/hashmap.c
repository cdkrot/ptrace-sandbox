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

#include "hashmap.h"
#include <linux/bug.h>
#include <linux/slab.h>

static bool is_prime(size_t n) {
    size_t i = 2;

    for (; i * i < n; ++i)
        if (n % i == 0)
            return false;

    return true;
}

int init_hashmap(struct hashmap* hmp, size_t min_size) {
    size_t real_size = min_size;
    size_t i;

    while (!is_prime(real_size))
        real_size += 1;

    hmp->used_size = 0;
    hmp->size      = real_size;
    hmp->req_size  = min_size;

    rwlock_init(&(hmp->lock));

    hmp->data = kmalloc(sizeof(struct hashmap_entry) * real_size, GFP_KERNEL);
    if (!(hmp->data))
        return -ENOMEM;

    for (i = 0; i != real_size; ++i)
        hmp->data[i].key = hmp->data[i].value = NULL;
    return 0;
}

/**
  * Returns:
  * index of interested entry (depends on mode)
  * or SIZE_MAX if no such.
  *
  * Doesn't acquire any locks!!!
  *
  * mode = 0 - find existing
  * mode = 1 - find good slot to insert.
  */
static size_t hashmap_find_unlocked(struct hashmap* hmp, const void* key, int mode) {
    size_t i;
    for (i = 0; i != hmp->tolerance; ++i) {
        size_t ind = hmp->hash(hmp, key, i);

        if (mode == 0 && hmp->equal(hmp->data[ind].key, key))
            return ind;

        if (hmp->data[ind].key == NULL) {
            if (mode == 1)
                return ind;
            if (mode == 0 && hmp->data[ind].value == NULL)
                return SIZE_MAX;
        }
    }

    return SIZE_MAX;
}

void* hashmap_get(struct hashmap* hmp, const void* key) {
    void* res = NULL;
    unsigned long flags;
    size_t pos;

    BUG_ON(key == NULL);
    read_lock_irqsave(&(hmp->lock), flags);

    pos = hashmap_find_unlocked(hmp, key, 0);
    if (pos != SIZE_MAX)
        res = hmp->data[pos].value;

    read_unlock_irqrestore(&(hmp->lock), flags);
    return res;
}

void* hashmap_erase(struct hashmap* hmp, const void* key) {
    void* res = NULL;
    unsigned long flags;
    size_t pos;

    BUG_ON(key == NULL);
    read_lock_irqsave(&(hmp->lock), flags);

    pos = hashmap_find_unlocked(hmp, key, 0);
    if (pos != SIZE_MAX) {
        res = hmp->data[pos].value;
        hmp->data[pos].key = NULL;
        hmp->data[pos].value = (void*)1; /* soo... dirt */

        hmp->used_size -= 1;
    }

    read_unlock_irqrestore(&(hmp->lock), flags);
    return res;
}

int hashmap_set(struct hashmap* hmp, const void* key, void* value, void** oldval) {
    if (value == NULL) {
        void* res = hashmap_erase(hmp, key);
        if (oldval)
            *oldval = res;
        return 0;
    } else {
        unsigned long flags;
        size_t pos;

        BUG_ON(key == NULL);
        read_lock_irqsave(&(hmp->lock), flags);

        if (oldval)
            *oldval = NULL;

        pos = hashmap_find_unlocked(hmp, key, 0);

        if (pos != SIZE_MAX && oldval)
            *oldval = hmp->data[pos].value;
        if (pos == SIZE_MAX) {
            pos = hashmap_find_unlocked(hmp, key, 1);
            if (pos == SIZE_MAX)
                return -ENOMEM;
            hmp->used_size += 1;
        }

        hmp->data[pos].key = key;
        hmp->data[pos].value = value;

        read_unlock_irqrestore(&(hmp->lock), flags);
        return 0;
    }
}
