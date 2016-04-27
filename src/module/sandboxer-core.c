//  Sandboxer, kernel module sandboxing stuff
//  Copyright (C) 2016  Vasiliy Alferov, Sayutin Dmitry
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/pid.h>
#include <linux/string.h>

#include "sandboxer-core.h"
#include "sandboxer-proc.h"
#include "sandboxer-mentor.h"
#include "sandboxer-slot.h"

MODULE_LICENSE("GPL");

struct kretprobe sys_mmap_kretprobe;
struct kprobe on_task_exit_kprobe;
struct jprobe syscall_probe;
struct jprobe sysret_probe;

// See fs/proc/array.c and fs/proc/task_mmu.c for the details
// Temporarily we need to have CONFIG_MMU enabled in kernel
unsigned long get_task_vm_size(struct task_struct* task) {
    unsigned long ret = 0;
    struct mm_struct *mm = get_task_mm(task);

    if (mm)
        ret = mm->total_vm;

    return ret * PAGE_SIZE;
}

int sandboxer_sys_mmap_return_handler(struct kretprobe_instance *ri, struct pt_regs* regs) {
    size_t mem;
    struct sandbox_slot *cur_slot;

    if (get_slot_of(current) != NOT_SANDBOXED) {
        printk(KERN_INFO "[sandboxer] now task memory is %lu\n", get_task_vm_size(current));

        mem = get_task_vm_size(current);
        cur_slot = get_slot_by_id(get_slot_of(current));
        cur_slot->mem_used = mem;
        if (mem > cur_slot->max_mem_used)
            cur_slot->max_mem_used = mem;
    }
    return 0; // Return value is currently ignored
}

unsigned long sandboxer_syscall_handler(struct pt_regs* regs, u32 arch) {
    if (get_slot_of(current) != NOT_SANDBOXED) {
        printk(KERN_INFO "syscall id: %lu\n", (unsigned long)(regs->orig_ax));
    }
    jprobe_return();
    return 0; /* never called */
}

void sandboxer_sysret_handler(struct pt_regs *regs) {
    if (get_slot_of(current) != NOT_SANDBOXED) {
        printk(KERN_INFO "sysreturn: %lu", regs_return_value(regs));
    }
    jprobe_return();
}

// do_exit
int sandboxer_exit_handler(struct kprobe* probe, struct pt_regs* regs) {
    if (get_slot_of(current) != NOT_SANDBOXED) {
        printk(KERN_INFO "Pid %d just died", current->pid);
        detach_task_from_slot();
    }
    return 0;
}

int init_or_shutdown_probes(bool init) {
    int errno = 0;
    if (init) {
        sys_mmap_kretprobe.kp.symbol_name = "sys_mmap";
        sys_mmap_kretprobe.handler = sandboxer_sys_mmap_return_handler;
        sys_mmap_kretprobe.maxactive = PID_MAX_LIMIT;
        if ((errno = register_kretprobe(&sys_mmap_kretprobe)) != 0) {
            printk(KERN_INFO "[sandboxer] ERROR: failed to create sys_mmap probe\n");
            goto out;
        }
        
        on_task_exit_kprobe.symbol_name = "do_exit";
        on_task_exit_kprobe.pre_handler = sandboxer_exit_handler;
        if ((errno = register_kprobe(&on_task_exit_kprobe)) != 0) {
            printk(KERN_INFO "[sandboxer] ERROR: failed to create do_exit probe\n");
            goto out_term_sys_mmap_probe;
        }
        
        syscall_probe.kp.symbol_name = "syscall_trace_enter_phase1";
        syscall_probe.entry = sandboxer_syscall_handler;
        if ((errno = register_jprobe(&syscall_probe)) != 0) {
            printk(KERN_INFO "[sandboxer] ERROR: failed to create syscall probe");
            goto out_term_do_exit_probe;
        }
    
        sysret_probe.kp.symbol_name = "syscall_return_slowpath";
        sysret_probe.entry = sandboxer_sysret_handler;
        if ((errno = register_jprobe(&sysret_probe)) != 0) {
            sysret_probe.kp.symbol_name = "syscall_trace_leave";
            if ((errno = register_jprobe(&sysret_probe)) != 0) {
                printk(KERN_INFO "[sandboxer] ERROR: failed to create syscall return probe");
                goto out_term_sys_enter_probe;
            }
        }
        
        return 0;
    }
    printk(KERN_INFO "[sandboxer] missed %d sys_mmap probe events\n", sys_mmap_kretprobe.nmissed);
    printk(KERN_INFO "[sandboxer] missed %lu do_exit probe events\n", on_task_exit_kprobe.nmissed);
    printk(KERN_INFO "[sandboxer] missed %lu syscall probe events\n", syscall_probe.kp.nmissed);
    printk(KERN_INFO "[sandboxer] missed %lu sysret  probe events\n", sysret_probe.kp.nmissed);
    unregister_jprobe(&sysret_probe);
out_term_sys_enter_probe:
    unregister_jprobe(&syscall_probe);
out_term_do_exit_probe:
    unregister_kprobe(&on_task_exit_kprobe);
out_term_sys_mmap_probe:
    unregister_kretprobe(&sys_mmap_kretprobe);
out:
    return errno;
}

int init_or_shutdown_module(bool is_init) {
    // TODO: provide generalized framework for such inits.
    int errno;

    errno = 0;
    
    if (is_init) {
        printk(KERN_INFO "[sandboxer] init\n");
        
        if ((errno = init_or_shutdown_slots(true)) != 0) {
            printk(KERN_INFO "[sandboxer] ERROR: failed to setup slots");
            goto out;
        }
        
        // Create /proc/sandboxer file
        if ((errno = sandboxer_init_proc()) != 0) {
            printk(KERN_INFO "[sandboxer] ERROR: failed to create /proc/sandboxer file\n");
            goto out_term_slots;
        }
        
        if ((errno = init_or_shutdown_probes(true)) != 0) {
            printk(KERN_INFO "[sandboxer] ERROR: failed to setup probes\n");
            goto out_term_proc;
        }
        
        if ((errno = init_mentor_stuff()) != 0) {
            printk(KERN_ERR "[sandboxer] ERROR: failed to init mentor stuff\n");
            goto out_term_probes;
        }

        return 0;
    }
out_term_probes:
    init_or_shutdown_probes(false);
out_term_proc:
    sandboxer_shutdown_proc();
out_term_slots:
    init_or_shutdown_slots(false);
out:
    return errno;
}

static int __init sandboxer_module_init(void) {
    return init_or_shutdown_module(true);
}
static void __exit sandboxer_module_exit(void) {
    printk(KERN_INFO "[sandboxer] shutting down\n");
    init_or_shutdown_module(false);
}

module_init(sandboxer_module_init);
module_exit(sandboxer_module_exit);
