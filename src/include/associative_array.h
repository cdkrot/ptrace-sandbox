#ifndef ASSOCIATIVE_ARRAY_H_
#define ASSOCIATIVE_ARRAY_H_

#include <stddef.h>
#include <stdint.h>

struct _s_associative_array;

struct _s_associative_array
{
    size_t key_size, val_size;
    int (*key_cmp)(const void*, const void*);
    void* key;
    void* val;
    int rnd;
    struct _s_associative_array* L;
    struct _s_associative_array* R;
};

typedef struct _s_associative_array* associative_array;

associative_array associative_array_init(size_t key_size, size_t val_size, int (*key_cmp)(const void*, const void*), void* key, void* val);

associative_array associative_array_add(associative_array v, associative_array to_a);
associative_array associative_array_find(associative_array v, void* key);
associative_array associative_array_remove(associative_array v, void* key);

#endif //ASSOCIATIVE_ARRAY_H_
