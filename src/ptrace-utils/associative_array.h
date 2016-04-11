//  Different utils
//  Copyright (C) 2016  Vasiliy Alferov
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
