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
#include "sandboxer-proc.h"
#include "sandboxer-core.h"

static const char SANDBOX_RESTRICTED[2] = "1";

static ssize_t sandboxer_proc_entry_write(struct file* _file, const char *buffer, 
    size_t length, loff_t *offset) {
    /* that is a temporary realization, used only to write one byte ('0' or '1') to /proc/sandboxer */
    if (*offset > 0)
        return 0;
    if (strcmp(buffer, SANDBOX_RESTRICTED) == 0) {
        u8 slot = create_new_slot();
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

int sandboxer_init_proc(void) {
    sandboxer_proc_entry = proc_create("sandboxer", 0666, NULL, &sandboxer_proc_entry_fops);
    if (sandboxer_proc_entry == NULL)
        return -EFAULT;
    return 0;
}

void sandboxer_shutdown_proc(void) {
    if (sandboxer_proc_entry != NULL)
        remove_proc_entry("sandboxer", NULL);
}
