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
    printk(KERN_INFO "Attached pid (%d) to slot (%u)\n", pid, (u32)slot);
}

void detach_pid_from_slot(pid_t pid) {
    BUG_ON(pid >= PID_MAX);
    BUG_ON(pid < 0);

    if (slot_of[pid] != NOT_SANDBOXED) {
        slots[slot_of[pid]].num_alive -= 1;

        if (slots[slot_of[pid]].num_alive == 0)
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

static int __init sandboxer_module_init(void) {
    int errno;
    
    printk(KERN_INFO "[sandboxer] init\n");
   
    sandboxer_init_slots();

    // Create /proc/sandboxer file
    errno = sandboxer_init_proc();
    if (errno) {
        printk(KERN_INFO "[sandboxer] ERROR: error while creating /proc/sandboxer file");
        return errno;
    }

    // Register kretprobe
    sys_mmap_kretprobe.kp.symbol_name = "sys_mmap";
    sys_mmap_kretprobe.handler = sandboxer_sys_mmap_return_handler;
    sys_mmap_kretprobe.maxactive = PID_MAX;
    errno = register_kretprobe(&sys_mmap_kretprobe);
    if (errno != 0) {
        printk(KERN_INFO "[sandboxer] ERROR: register_kretprobe returned %d.\n", errno);
        sandboxer_shutdown_proc();
        return errno;
    }

    return 0;
}

static void __exit sandboxer_module_exit(void) {
    printk(KERN_INFO "[sandboxer] missed %d probe events\n", sys_mmap_kretprobe.nmissed);
    printk(KERN_INFO "[sandboxer] exit\n");
    unregister_kretprobe(&sys_mmap_kretprobe);
    sandboxer_shutdown_proc();
}

module_init(sandboxer_module_init);
module_exit(sandboxer_module_exit);
