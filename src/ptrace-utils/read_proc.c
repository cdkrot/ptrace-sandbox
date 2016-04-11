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


#include <stdio.h>

#include "read_proc.h"

struct statm_info get_process_statm_info(pid_t pid) {
    struct statm_info ret;
    char filename[100];
    sprintf(filename, "/proc/%d/statm", pid);
    FILE* procstatm = fopen(filename, "r");
    fscanf(procstatm, "%lu %lu %lu %lu %lu %lu %lu", &ret.size, &ret.resident, &ret.share, &ret.text, &ret.lib, &ret.data, &ret.dt);
    fclose(procstatm);
    return ret;
}
