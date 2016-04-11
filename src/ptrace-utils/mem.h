//  Different utils
//  Copyright (C) 2016  Sayutin Dmitry
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
