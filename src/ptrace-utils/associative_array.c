#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <associative_array.h>

associative_array associative_array_init(size_t key_size, size_t val_size, int (*key_cmp)(const void*, const void*), void* key, void* val)
{
    associative_array ret = malloc(sizeof(struct _s_associative_array));
    ret->key_size = key_size;
    ret->val_size = val_size;
    ret->key_cmp = key_cmp;
    ret->key = malloc(key_size);
    memcpy(ret->key, key, key_size);
    ret->val = malloc(val_size);
    memcpy(ret->val, val, val_size);
    ret->rnd = rand();
    ret->L = NULL;
    ret->R = NULL;
    return ret;
}

static associative_array merge(associative_array left, associative_array right)
{
    if (!left)
        return right;
    if (!right)
        return left;
    if (left->rnd > right->rnd)
    {
        left->R = merge(left->R, right);
        return left;
    }
    else
    {
        right->L = merge(left, right->L);
        return right;
    }
}

static void split(associative_array v, void* x, associative_array* left, associative_array* right)
{
    if (!v)
    {
        *left = NULL;
        *right = NULL;
    }
    else if (v->key_cmp(v->key, x) == -1)
    {
        split(v->R, x, &(v->R), right);
        *left = v;
    }
    else
    {
        split(v->L, x, left, &(v->L));
        *right = v;
    }
}

associative_array associative_array_add(associative_array v, associative_array to_a)
{
    if (!v)
        return to_a;
    if (to_a->rnd > v->rnd)
    {
        split(v, to_a->key, &(to_a->L), &(to_a->R));
        return to_a;
    }
    else if (v->key_cmp(to_a->key, v->key) >= 0)
        v->R = associative_array_add(v->R, to_a);
    else
        v->L = associative_array_add(v->L, to_a);
    return v;
}

associative_array associative_array_find(associative_array v, void* key)
{
    if (!v)
        return NULL;
    int a = v->key_cmp(v->key, key);
    if (a == 0)
        return v;
    else if (a > 0)
        return associative_array_find(v->L, key);
    else
        return associative_array_find(v->R, key);
}

associative_array associative_array_remove(associative_array v, void* key)
{
    if (!v)
        return NULL;
    int a = v->key_cmp(v->key, key);
    if (a == 0)
        return merge(v->L, v->R);
    else if (a > 0)
        v->L = associative_array_remove(v->L, key);
    else
        v->R = associative_array_remove(v->R, key);
    return v;
}
