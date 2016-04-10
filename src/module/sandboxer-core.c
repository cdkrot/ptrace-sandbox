#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/pid.h>
#include <linux/string.h>

#include "sandboxer-proc.h"
#include "sandboxer-core.h"

MODULE_LICENSE("GPL");

u8 slot_of[PID_MAX];
struct sandbox_slot slots[NUM_SANDBOXING_SLOTS];

struct kretprobe sys_mmap_kretprobe;
struct kprobe on_task_exit_kprobe;

// stack of open slots.
u8 free_slots[NUM_SANDBOXING_SLOTS];
size_t p_free_slot = NUM_SANDBOXING_SLOTS;

void sandboxer_init_slots(void) {
    u8 i;
    
    memset(slot_of, NOT_SANDBOXED, PID_MAX);
    
    for (i = 0; i != NUM_SANDBOXING_SLOTS; ++i)
        free_slots[i] = NUM_SANDBOXING_SLOTS - 1 - i;
}

u8 create_new_slot(void) {
    u8 res;
    if (p_free_slot == 0)
        return NOT_SANDBOXED;
    res = free_slots[--p_free_slot];
    
    slots[res].num_alive = 0;
    slots[res].memory_used = 0;
    slots[res].max_memory_used = 0;

    printk(KERN_INFO "Allocated new sandboxing slot (%u)\n", (u32)res);
    return res;
}

void release_slot(u8 slot) {
    free_slots[p_free_slot++] = slot;
}

void attach_pid_to_slot(pid_t pid, u8 slot) {
    BUG_ON(pid >= PID_MAX);
    BUG_ON(pid < 0);
    BUG_ON(slot >= NUM_SANDBOXING_SLOTS);
    BUG_ON(slot_of[pid] != NOT_SANDBOXED);

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

static int __init sandboxer_module_init(void) {
    int errno;
    
    printk(KERN_INFO "[sandboxer] init\n");
   
    sandboxer_init_slots();

    // Create /proc/sandboxer file
    if ((errno = sandboxer_init_proc()) != 0) {
        printk(KERN_INFO "[sandboxer] ERROR: failed to create /proc/sandboxer file\n");
        goto out;
    }

    // Register kretprobe [sys_mmap]
    sys_mmap_kretprobe.kp.symbol_name = "sys_mmap";
    sys_mmap_kretprobe.handler = sandboxer_sys_mmap_return_handler;
    sys_mmap_kretprobe.maxactive = PID_MAX;
    if ((errno = register_kretprobe(&sys_mmap_kretprobe)) != 0) {
        printk(KERN_INFO "[sandboxer] ERROR: failed to create sys_mmap probe\n");
        goto out_term_proc;
    }

    // register kretprobe [do_exit]
    on_task_exit_kprobe.symbol_name = "do_exit";
    on_task_exit_kprobe.pre_handler = sandboxer_exit_handler;
//    on_task_exit_kprobe.maxactive = PID_MAX; /* no such field */
    if ((errno = register_kprobe(&on_task_exit_kprobe)) != 0) {
        printk(KERN_INFO "[sandboxer] ERROR: failed to create do_exit probe\n");
        goto out_term_sys_mmap_probe;
    }

    return 0;
    
//out_term_do_exit_probe:
//    unregister_kprobe(&on_task_exit_kprobe);
out_term_sys_mmap_probe:
    unregister_kretprobe(&sys_mmap_kretprobe);
out_term_proc:
    sandboxer_shutdown_proc();
out:
    printk(KERN_INFO "[sandboxer] FATAL_ERROR: failed to initialize, errno = %d\n", errno);
    return errno;
}

static void __exit sandboxer_module_exit(void) {
    printk(KERN_INFO "[sandboxer] missed %d sys_mmap probe events\n", sys_mmap_kretprobe.nmissed);
    printk(KERN_INFO "[sandboxer] missed %lu do_exit probe events\n", on_task_exit_kprobe.nmissed);
    
    printk(KERN_INFO "[sandboxer] exit\n");
    unregister_kretprobe(&sys_mmap_kretprobe);
    unregister_kprobe(&on_task_exit_kprobe);
    sandboxer_shutdown_proc();
}

module_init(sandboxer_module_init);
module_exit(sandboxer_module_exit);
