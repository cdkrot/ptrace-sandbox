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

#include "proc.h"
#include "slot.h"
#include "notifications.h"

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

/* /proc/sandboxer/sandbox_me behaviour implementation: */

static const char SANDBOX_RESTICTED = '1';

static ssize_t sandbox_me_write(struct file *file, const char *buffer, size_t length, loff_t *offset) {
    if (*offset > 0)
        return 0;
    if (length == 1 && *buffer == SANDBOX_RESTICTED) {
        if (create_slot() == NULL)
            return -ENOMEM;
    } else
        return -EINVAL;
    *offset = 1;
    return 1;
}

static void *sandbox_me_seq_start(struct seq_file *s, loff_t *pos) {
    struct sandbox_slot *slot;

    if (*pos > 0 || (slot = get_slot_of()) == NULL)
        *pos = 0;

    return slot;
}

static int sandbox_me_seq_show(struct seq_file *s, void *v) {
    struct sandbox_slot *slot = v;

    seq_printf(s, "%lu", slot->slot_id);
    return 0;
}

static void *sandbox_me_seq_next(struct seq_file *s, void *v, loff_t *pos) {
    ++*pos;
    return NULL;
}

static void sandbox_me_seq_stop(struct seq_file *s, void *v) {}

static const struct seq_operations sandbox_me_seq_ops = {
    .start = sandbox_me_seq_start,
    .show = sandbox_me_seq_show,
    .next = sandbox_me_seq_next,
    .stop = sandbox_me_seq_stop
};

static int sandbox_me_open(struct inode *inode, struct file *file) {
    return seq_open(file, &sandbox_me_seq_ops);
}

static const struct file_operations sandbox_me_file_ops = {
    .owner = THIS_MODULE,
    .open = sandbox_me_open,
    .write = sandbox_me_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release
};

// /proc/sandboxer/notifications behaviour implementation:

static void *notifications_seq_start(struct seq_file *s, loff_t *pos) {
    struct notification *n;
    int errno;

    if (*pos > 0) {
        *pos = 0;
        return NULL;
    }

    // Well, notification should be stored somewhere, right?
    n = kmalloc(sizeof(struct notification), GFP_KERNEL);
    if (!n) {
        printk(KERN_INFO "sandboxer: could not allocate memory for notification");
        return NULL;
    }

    errno = read_notification(task_pid(current), n);
    if (errno) {
        printk(KERN_INFO "sandboxer: read_notification exited with errno %d", errno);
        return NULL;
    }

    return n;
}

static int notifications_seq_show(struct seq_file *s, void *v) {
    struct notification *n = v;

    switch (n->type) {
        case SLOT_CREATE:
            seq_printf(s, "SLOT_CREATE %d\n", n->data.slot_create.slot_id);
            break;

        case SLOT_STOP:
            seq_printf(s, "SLOT_STOP %d %s\n", n->data.slot_stop.slot_id, n->data.slot_stop.reason);
            kfree(n->data.slot_stop.reason);
            break;

        case SLOT_TERM:
            seq_printf(s, "SLOT_TERM %d\n", n->data.slot_term.slot_id);
            break;

        case SEC_VIOL:
            seq_printf(s, "SEC_VIOL %d\n", n->data.sec_viol.slot_id);
            break;

        case MEM_LIM:
            seq_printf(s, "MEM_LIM %d\n", n->data.mem_lim.slot_id);
            break;
    }

    return 0;
}

static void *notifications_seq_next(struct seq_file *s, void *v, loff_t *pos) {
    ++*pos;
    return NULL;
}

static void notifications_seq_stop(struct seq_file *s, void *v) {
    kfree(v);
}

static const struct seq_operations notifications_seq_ops = {
    .start = notifications_seq_start,
    .show = notifications_seq_show,
    .next = notifications_seq_next,
    .stop = notifications_seq_stop
};

static int notifications_open(struct inode *inode, struct file *file) {
    return seq_open(file, &notifications_seq_ops);
}

static const struct file_operations notifications_file_ops = {
    .owner = THIS_MODULE,
    .open = notifications_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release
};

static struct proc_dir_entry *sandboxer_dir;
static struct proc_dir_entry *sandbox_me_entry;
static struct proc_dir_entry *notifications_entry;

#define FAIL_ON_NULL(x)                               \
    if ((x) == NULL) {                                \
        init_or_shutdown_sandboxer_proc_dir(0, NULL); \
        return -ENOMEM;                               \
    }

int init_or_shutdown_sandboxer_proc_dir(int initlib_mode, __attribute__((unused)) void *ignored) {
    if (initlib_mode) {
        FAIL_ON_NULL(sandboxer_dir = proc_mkdir("sandboxer", NULL))

        FAIL_ON_NULL(sandbox_me_entry = proc_create("sandbox_me", 0666, sandboxer_dir, &sandbox_me_file_ops))

        FAIL_ON_NULL(notifications_entry = proc_create("notifications", 0444, sandboxer_dir,
                                                       &notifications_file_ops))
    } else {
        if (sandbox_me_entry) {
            remove_proc_entry("sandbox_me", sandboxer_dir);
        }
        if (notifications_entry) {
            remove_proc_entry("notifications", sandboxer_dir);
        }
        if (sandboxer_dir) {
            remove_proc_entry("sandboxer", NULL);
        }
    }
    return 0;
}
