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

#ifndef SANDBOXER_HASHMAP_H_
#define SANDBOXER_HASHMAP_H_

#include <linux/spinlock.h>
#include <linux/types.h>

/**
  * Hashmap entry.
  *
  * However key or value NULL has special meaning.
  * if key == NULL and value == NULL, then this slot is empty
  * if key == NULL and value != NULL, then this slot is dirt.
  */
struct hashmap_entry {
    const void *key;
    void *value;
};

/**
  * Main hashmap struct.
  * hashmap is map from void* to void*.
  *
  * hashmap uses user provided hash and equal functions.
  *
  * hashmap _is_ thread safe.
  *
  * Please use functions below to work with hashmap:
  */
struct hashmap {
    /** raw hashmap data */
    struct hashmap_entry* data;

    /** num of used slots in data (dirt slots are excluded) */
    size_t used_size;
    /** size of data array */
    size_t size;
    /** requested hashmap size. is is <= than size */
    size_t req_size;

    /** maximum number of viewed entries before giving up.
      * worst performance is proporational to this number.
      *
      * however than bigger tolerance, than more requests will be served successfully.
      * Don't change it since you started to use hashmap.
      */
    size_t tolerance;

    /** global lock on a hashmap */
    rwlock_t lock;

    /** Pointer to user hashing function
      * Hashing function takes hashmap pointer, key, and try number [0, tolerance)
      * It should return the hash modulos hashmap->size.
      */
    size_t (*hash)(const struct hashmap*, const void*, size_t tryid);

    /** Pointer to user comparator
      * Given two keys it should return true if and only if they are equal.
      */
    bool (*equal)(const void*, const void*);
};

/**
  * Creates hashmap with size at least min_size.
  *
  * Actually, it initializes following hashmap fields:
  * data, used_size, size, req_size, lock.
  *
  * You still need to provide hash and equal functions, and "tolerance" param.
  * (It can be done prior or after execution of this function)
  *
  * Returns non-zero error code on error.
  */
int init_hashmap(struct hashmap* hmp, size_t min_size);

/**
  * Returns value, related to the key. Or null if no such key.
  */
void* hashmap_get(struct hashmap* hmp, const void* key);


/**
  * Erases entry with requested key
  * Returns result of get(key). You probably want to call kfree() on
  * value anyway.
  */
void* hashmap_erase(struct hashmap* hmp, const void* key);

/**
  * Cleans memory used by hashmap, notice that
  * It _doesn't_ calls kfree on keys or values.
  */
void hashmap_free(struct hashmap* hmp);

/**
  * Sets the new value for key, if no value yet, than it's created.
  * if oldval != NULL, than *oldval is assigned with old value of key (or NULL, if no such).
  * Returns non-zero errno on failure with inserting new key.
  */
int hashmap_set(struct hashmap* hmp, const void* key, void* value, void** oldval);

#endif
