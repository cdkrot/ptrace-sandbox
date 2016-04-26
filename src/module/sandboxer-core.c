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

#include "sandboxer-proc.h"
#include "sandboxer-core.h"
#include "sandboxer-mentor.h"

MODULE_LICENSE("GPL");

u8 slot_of[PID_MAX];
struct sandbox_slot slots[NUM_SANDBOXING_SLOTS];
struct slot_id_info allocated_slot_ids[PID_MAX];

struct kretprobe sys_mmap_kretprobe;
struct kprobe on_task_exit_kprobe;
struct jprobe syscall_probe;
struct jprobe sysret_probe;

// stack of open slots.
u8 free_slots[NUM_SANDBOXING_SLOTS];
size_t p_free_slot = NUM_SANDBOXING_SLOTS;
DEFINE_SPINLOCK(stack_lock);

void sandboxer_init_slots(void) {
    u8 i;    
    
    memset(slot_of, NOT_SANDBOXED, PID_MAX);
    
    for (i = 0; i != NUM_SANDBOXING_SLOTS; ++i)
        free_slots[i] = NUM_SANDBOXING_SLOTS - 1 - i;
}

u8 create_new_slot(pid_t mentor) {
    u8 res;
    struct mentor_stuff *ms;
    struct slot_id_info *info;

    spin_lock(&stack_lock);
    
    if (p_free_slot == 0) {
        spin_unlock(&stack_lock);
        return NOT_SANDBOXED;
    }

    res = free_slots[--p_free_slot];

    spin_unlock(&stack_lock);
    
    slots[res].mentor = mentor;
    slots[res].num_alive = 0;
    slots[res].memory_used = 0;
    slots[res].max_memory_used = 0;
    
    info = kmalloc(sizeof(struct slot_id_info), GFP_ATOMIC);
    if (!info)
        goto out_release_slot;
    
    info->slot_id = res;
    
    ms = manage_mentor_stuff(mentor, MENTOR_GET_OR_CREATE);
    if (ms == NULL)
        goto out_release_info;

    spin_lock(&(ms->lock));
    
    llist_add(&(info->llnode), &(ms->awaited_slot_ids));
    if (waitqueue_active(&(ms->info_wq))) 
        wake_up_interruptible(&(ms->info_wq));

    spin_unlock(&(ms->lock));
    
    printk(KERN_INFO "Added an llnode to llist located at %p\n", &(ms->awaited_slot_ids));        
    printk(KERN_INFO "Allocated new sandboxing slot (%u; mentor is %d)\n", (u32)res, mentor);

    return res;
out_release_info:
    kfree(info);
out_release_slot:
    release_slot(res);
    return NOT_SANDBOXED;
}

void release_slot(u8 slot) {
    spin_lock(&stack_lock);
    free_slots[p_free_slot++] = slot;
    printk(KERN_INFO "Slot %d released, total %zu slots available now\n", (u32)slot, p_free_slot);
    spin_unlock(&stack_lock);
}

void attach_pid_to_slot(pid_t pid, u8 slot) {
    BUG_ON(pid >= PID_MAX);
    BUG_ON(pid < 0);
    BUG_ON(slot >= NUM_SANDBOXING_SLOTS);
    BUG_ON(slot_of[pid] != NOT_SANDBOXED);

    /* we want to recieve all events related to syscall enter.
       so we set this flag.
       Setting tif_seccomp results in panic()... =( */
    set_thread_flag(TIF_SYSCALL_AUDIT);
    
    slot_of[pid] = slot;
    slots[slot].num_alive += 1;
    slots[slot].ref_cnt += 1;
    printk(KERN_INFO "Attached pid (%d) to slot (%u)\n", pid, (u32)slot);
}

void detach_pid_from_slot(pid_t pid) {
    BUG_ON(pid >= PID_MAX);
    BUG_ON(pid < 0);

    if (slot_of[pid] != NOT_SANDBOXED) {
        slots[slot_of[pid]].num_alive -= 1;
        slots[slot_of[pid]].ref_cnt -= 1;
        
        if (slots[slot_of[pid]].ref_cnt == 0)
            release_slot(slot_of[pid]);
        
        slot_of[pid] = NOT_SANDBOXED;
    }
}

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

    if (slot_of[current->pid] != NOT_SANDBOXED) {
        printk(KERN_INFO "[sandboxer] sys_mmap return handled. Return value is %lu\n", (unsigned long)(regs_return_value(regs)));
        printk(KERN_INFO "[sandboxer] now task memory is %lu\n", get_task_vm_size(current));

        mem = get_task_vm_size(current);
        cur_slot = slots + slot_of[current->pid];
        cur_slot->memory_used = mem;
        if (mem > cur_slot->max_memory_used)
            cur_slot->max_memory_used = mem;
    }
    return 0; // Return value is currently ignored
}

unsigned long sandboxer_syscall_handler(struct pt_regs* regs, u32 arch) {
    if (slot_of[current->pid] != NOT_SANDBOXED) {
        printk(KERN_INFO "syscall id: %lu\n", (unsigned long)(regs->orig_ax));
    }
    jprobe_return();
    return 0; /* never called */
}

void sandboxer_sysret_handler(struct pt_regs *regs) {
    if (slot_of[current->pid] != NOT_SANDBOXED) {
        printk(KERN_INFO "sysreturn: %lu", regs_return_value(regs));
    }
    jprobe_return();
}

static void sandboxer_release_handler(struct task_struct* tsk);

// do_exit
int sandboxer_exit_handler(struct kprobe* probe, struct pt_regs* regs) {
    if (slot_of[current->pid] != NOT_SANDBOXED) {
        printk(KERN_INFO "Pid %d is now zombie", current->pid);
        slots[slot_of[current->pid]].num_alive -= 1;
        sandboxer_release_handler(current); // at the moment coexist together.
    }
    return 0;
}

void sandboxer_release_handler(struct task_struct* tsk) {
    if (slot_of[tsk->pid] != NOT_SANDBOXED) {
        printk(KERN_INFO "Pid %d is leaving zombie state\n", tsk->pid);
        slots[slot_of[tsk->pid]].ref_cnt -= 1;
        if (slots[slot_of[tsk->pid]].ref_cnt == 0)
            release_slot(slot_of[tsk->pid]);
        slot_of[tsk->pid] = NOT_SANDBOXED;
    }
}

int init_or_shutdown_probes(bool init) {
    int errno = 0;
    if (init) {    
        sys_mmap_kretprobe.kp.symbol_name = "sys_mmap";
        sys_mmap_kretprobe.handler = sandboxer_sys_mmap_return_handler;
        sys_mmap_kretprobe.maxactive = PID_MAX;
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

static int __init sandboxer_module_init(void) {
    int errno;
    
    printk(KERN_INFO "[sandboxer] init\n");
  
    sandboxer_init_slots();

    // Create /proc/sandboxer file
    if ((errno = sandboxer_init_proc()) != 0) {
        printk(KERN_INFO "[sandboxer] ERROR: failed to create /proc/sandboxer file\n");
        goto out;
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
out_term_probes:
    init_or_shutdown_probes(false);
out_term_proc:
    sandboxer_shutdown_proc();
out:
    printk(KERN_INFO "[sandboxer] FATAL_ERROR: failed to initialize, errno = %d\n", errno);
    return errno;
}

static void __exit sandboxer_module_exit(void) {
    printk(KERN_INFO "[sandboxer] shutting down\n");
    shutdown_mentor_stuff();
    init_or_shutdown_probes(false);
    sandboxer_shutdown_proc();
}

module_init(sandboxer_module_init);
module_exit(sandboxer_module_exit);
