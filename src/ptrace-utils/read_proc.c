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
