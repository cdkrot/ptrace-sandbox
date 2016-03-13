/* src/module/hashmap.h : hashmap implementation
 *
 * Provides a hashmap with open addressing and double hashing. The hashmap has
 * size HASHMAP_SIZE which must be #defined before including that file, A good
 * choice of HASHMAP_SIZE would be a prime number bigger enough than its real
 * expected capacity. The hashmap maps integers to pointers(void*). First hash
 * function is simply h1(x) = x % HASMAP_SIZE, second hash function (used to
 * address values in case of collision) is (yeah, we love that pretty constant) 
 * h2(x) = (179 * x) % (HASHMAP_SIZE - 1) + 1.
 *
 * Complexity is O(HASHMAP_SIZE) in the wthe worst case, and O(1) at average.
 * Hashmap size is (sizeof(int) + sizeof(void*)) * HASHMAP_SIZE.
 *
 * Warning: keys are _supposed_ to be non-negative. So inserting a negative key
 * will do nothing.
 *
 */

#ifndef SANDBOXER_HASHMAP_H_
#define SANDBOXER_HASHMAP_H_

#ifndef HASHMAP_SIZE
#error You must #define HASHMAP_SIZE before including this file
#endif

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdbool.h>
#endif

int hashmap_h1(int x)
{
    return x % HASHMAP_SIZE;
}

int hashmap_h2(int x)
{
    return (179 * x) % (HASHMAP_SIZE - 1) + 1;
}

/* Internally:
 *
 * key = -1 if element is empty;
 * key = -2 if element has been deleted.
 *
 */

struct hashmap
{
    int keys[HASHMAP_SIZE];
    void* values[HASHMAP_SIZE];
};

void hashmap_init(struct hashmap* hm)
{
    for (int i = 0; i < HASHMAP_SIZE; i++)
    {
        hm->keys[i] = -1;
        hm->values[i] = NULL;
    }
}

void hashmap_set(struct hashmap* hm, int key, void* val)
{
    if (key < 0)
        return;
    int cur = hashmap_h1(key), step = hashmap_h2(key);
    while (hm->keys[cur] != -1 && hm->keys[cur] != -2 && hm->keys[cur] != key)
        cur = (cur + step) % HASHMAP_SIZE;
    hm->keys[cur] = key;
    hm->values[cur] = val;
}

void* hashmap_val(struct hashmap* hm, int key)
{
    if (key < 0)
        return NULL;
    int cur = hashmap_h1(key), step = hashmap_h2(key);
    while (hm->keys[cur] != -1 && hm->keys[cur] != key)
        cur = (cur + step) % HASHMAP_SIZE;
    return hm->values[cur];
}

void hashmap_remove(struct hashmap* hm, int key)
{
    if (key < 0)
        return;
    int cur = hashmap_h1(key), step = hashmap_h2(key);
    while (hm->keys[cur] != -1 && hm->keys[cur] != key)
        cur = (cur + step) % HASHMAP_SIZE;
    if (hm->keys[cur] == key)
    {
        hm->keys[cur] = -2;
        hm->values[cur] = NULL;
    }
}

#endif //SANDBOXER_HASHMAP_H_
