#ifndef PT_SANDBOX_UTILS_MEM_H
#define PT_SANDBOX_UTILS_MEM_H

#include "attributes.h"

#define DECLARE_ALLOCATION(T) \
    T* TEMPLATED(allocate_objs, T)(size_t cnt) { \
        return (T*)(calloc(cnt, sizeof(T)));
    } \
    void TEMPLATED(deallocate_objs, T)(T* ptr, size_t cnt) { \
        free(ptr);
    } \

#define 
