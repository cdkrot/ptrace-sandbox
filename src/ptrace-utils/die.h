#ifndef PT_UTIL_DIE_H
#define PT_UTIL_DIE_H

__attribute__((noreturn)) void die(int exit_code, const char* fmt, ...);
void check_errno(int exit_code);

#endif
