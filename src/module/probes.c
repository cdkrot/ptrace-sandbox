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

#include <linux/kprobes.h>
#include "init.h"
#include <linux/sched.h>
#include "probes.h"

struct kprobe probe_task_exit;
struct jprobe probe_syscall_enter;
struct jprobe probe_syscall_leave;

static int sandboxer_on_task_died(struct kprobe* kp, struct pt_regs* regs) {
    return 0;
}

unsigned long sandboxer_on_syscall(struct pt_regs* regs, u32 arch) {
    jprobe_return();
    return 0; /* never called */
}

void sandboxer_on_sysleave(struct pt_regs* regs) {
    jprobe_return();
}

int sandboxer_init_probes(void) {
    probe_task_exit.symbol_name = "do_exit";
    probe_task_exit.pre_handler = sandboxer_on_task_died;
    initlib_push_errmsg(init_or_shutdown_kprobe, &probe_task_exit,
                        "sandboxer: failed to initialize probe on `do_exit`");

    probe_syscall_enter.kp.symbol_name = "syscall_trace_enter_phase1";
    probe_syscall_enter.entry          = sandboxer_on_syscall;
    initlib_push_errmsg(init_or_shutdown_jprobe, &probe_syscall_enter,
                        "sandboxer: failed to initialize probe on syscall enter");

    probe_syscall_leave.kp.symbol_name = "syscall_return_slowpath";
    probe_syscall_leave.entry          = sandboxer_on_sysleave;
    if (initlib_push_advanced(init_or_shutdown_jprobe, &probe_syscall_leave,
                              NULL, false)) {
        printk("sandboxer: trying to fallback to oldschool syscall leave handler");
        probe_syscall_leave.kp.symbol_name = "syscall_trace_leave";
        initlib_push_errmsg(init_or_shutdown_jprobe, &probe_syscall_leave,
                            "sandboxer: failed to initialized probe on syscall leave");
    }

    return 0;
}

int init_or_shutdown_kprobe(int is_init, void *kp) {
    if (is_init)
        return register_kprobe((struct kprobe*) kp);
    unregister_kprobe((struct kprobe*) kp);
    return 0;
}

int init_or_shutdown_jprobe(int is_init, void *jp) {
    if (is_init)
        return register_jprobe((struct jprobe*) jp);
    unregister_jprobe((struct jprobe*) jp);
    return 0;
}

int init_or_shutdown_kretprobe(int is_init, void *krp) {
    if (is_init)
        return register_kretprobe((struct kretprobe*) krp);
    unregister_kretprobe((struct kretprobe*) krp);
    return 0;
}
