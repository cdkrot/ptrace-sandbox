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
#include <linux/sched.h>
#include "init.h"
#include "probes.h"
#include "slot.h"
#include "notifications.h"

// Include tag handler headers

#include "tags/fork.h"

struct kprobe probe_task_exit;
struct jprobe probe_syscall_enter;
struct jprobe probe_syscall_leave;

static int sandboxer_on_task_died(struct kprobe* kp, struct pt_regs* regs) {
    release_slot();
    on_mentor_died(task_pid(current));
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

    /* syscall_trace_leave was removed in kernel commit 88cd622. We probe it only in older kernels.
     * syscall_return_slowpath was introduced in kernel 4.2.0-rc4. */
    if (kallsyms_lookup_name("syscall_return_slowpath")) {
        probe_syscall_leave.kp.symbol_name = "syscall_return_slowpath";
    } else {
        printk(KERN_INFO "sandboxer: syscall_return_slowpath not found, "
                         "falling back to syscall_trace_leave\n");
        probe_syscall_leave.kp.symbol_name = "syscall_trace_leave";
    }
    probe_syscall_leave.entry          = sandboxer_on_sysleave;
    initlib_push_errmsg(init_or_shutdown_jprobe, &probe_syscall_leave,
                        "sandboxer: failed to initialized probe on syscall leave");

    // Init tag handlers
    init_fork_tag();

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
