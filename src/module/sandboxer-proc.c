//  Sandboxer, kernel module sandboxing stuff
//  Copyright (C) 2016  Sayutin Dmitry, Vasiliy Alferov
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

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include "sandboxer-proc.h"
#include "sandboxer-core.h"
#include "sandboxer-mentor.h"

static const char SANDBOX_RESTRICTED[2] = "1";

static ssize_t sandboxer_proc_entry_write(struct file* _file, const char *buffer, 
    size_t length, loff_t *offset) {
    /* that is a temporary realization, used only to write one byte ('0' or '1') to /proc/sandboxer */
    u8 slot;

    if (*offset > 0)
        return 0;
    if (strcmp(buffer, SANDBOX_RESTRICTED) == 0) {
        if (current->parent == NULL) {
            /* application should be sandboxed by its parent */
            return -EFAULT;
        }
        slot = create_new_slot(current->parent->pid);
        if (slot == NOT_SANDBOXED)
            return -EFAULT;
        else
            attach_pid_to_slot(current->pid, slot);
    } else
        return -EFAULT;
    *offset = 2;
    return 2;
}

static void* sandboxer_seq_start(struct seq_file *s, loff_t *pos) {
    static struct sandbox_slot* slot;

    if (*pos == 0 && slot_of[current->pid] != NOT_SANDBOXED)
        slot = &(slots[slot_of[current->pid]]);
    else {
        *pos = 0;
        slot = NULL;
    }
    
    return slot;
}

static int sandboxer_seq_show(struct seq_file *s, void *v) {
    struct sandbox_slot* slot = v;

    seq_printf(s, "%lu", slot->max_memory_used);
    return 0;
}

static void* sandboxer_seq_next(struct seq_file *s, void *v, loff_t *pos) {
    ++*pos;
    return NULL;
}

static void sandboxer_seq_stop(struct seq_file *s, void *v) {}

static const struct seq_operations sandboxer_seq_ops = {
    .start = sandboxer_seq_start,
    .next = sandboxer_seq_next,
    .stop = sandboxer_seq_stop,
    .show = sandboxer_seq_show
};

static int sandboxer_proc_entry_open(struct inode *_inode, struct file *_file) {
    return seq_open(_file, &sandboxer_seq_ops);
}

struct proc_dir_entry *sandboxer_proc_entry;

static const struct file_operations sandboxer_proc_entry_fops = {
    .owner = THIS_MODULE,
    .open = sandboxer_proc_entry_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
    .write = sandboxer_proc_entry_write
};

static ssize_t sandboxer_info_read (struct file *_file, char __user *v, size_t count, loff_t *pos) {
    struct llist_node* llnode;
    struct slot_id_info* info;
    struct mentor_stuff* ms;

    printk(KERN_INFO "sandboxer_info_read called");

    if (*pos == 1) {
        // We have already told all information to the process.
        return 0;
    }

    ms = get_mentor_stuff(current->pid);
    if (!ms) {
        ms = create_mentor_stuff(current->pid);
        ms->awaited_slot_ids.first = NULL;
        spin_lock_init(&(ms->awaited_lock));
        INIT_WAIT_QUEUE_HEAD(ms->info_wq);
    }

    BUG_ON(!ms);

    if (llist_empty(&(ms->awaited_slot_ids))) {
        // Then current task will sleep till we have something to tell him.
        printk(KERN_INFO "Process %d now sleeps (waiting for llist located in %p)\n", current->pid, &(ms->awaited_slot_ids));
        wait_event_interruptible(ms->info_wq, !llist_empty(&(ms->awaited_slot_ids)));
        printk(KERN_INFO "Process %d woke up\n", current->pid);
    }
    // Now we have something to tell current task.
    printk(KERN_INFO "Setting up a spinlock for %d.\n", current->pid);
    spin_lock(&(ms->awaited_lock));
    BUG_ON(llist_empty(&(ms->awaited_slot_ids)));
    llnode = llist_del_first(&(ms->awaited_slot_ids));
    spin_unlock(&(ms->awaited_lock));

    BUG_ON(!llnode);
    info = llist_entry(llnode, struct slot_id_info, llnode);
    BUG_ON(!info);
    *v = (char)info->slot_id;
    llnode = NULL;
    kfree(info);
    info = NULL;
    *pos = 1;
    return 1;
};

static const struct file_operations sandboxer_info_fops = {
    .owner = THIS_MODULE,
    .read = sandboxer_info_read
};

static struct proc_dir_entry *sandboxer_info_proc_entry;

int sandboxer_init_proc(void) {
    sandboxer_proc_entry = proc_create("sandboxer", 0666, NULL, &sandboxer_proc_entry_fops);
    if (sandboxer_proc_entry == NULL)
        return -EFAULT;

    sandboxer_info_proc_entry = proc_create("sandboxer_info", 0444, NULL, &sandboxer_info_fops);
    if (sandboxer_info_proc_entry == NULL)
        return -EFAULT;

    return 0;
}

void sandboxer_shutdown_proc(void) {
    if (sandboxer_proc_entry != NULL)
        remove_proc_entry("sandboxer", NULL);
    if (sandboxer_info_proc_entry != NULL)
        remove_proc_entry("sandboxer_info", NULL);
}
