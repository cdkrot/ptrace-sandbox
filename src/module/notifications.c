//  Sandboxer, kernel module sandboxing stuff
//  Copyright (C) 2016  Alferov Vasiliy, Sayutin Dmitry
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

#include "splay.h"
#include "notifications.h"

#include <linux/pid.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>

// Module param:
static int notification_queue_size = 50;
module_param(notification_queue_size, int, 0000);

// Code related to the tree:

struct notification_queue {
    struct pid *pid;
    struct splay_tree_node node;
    
    int ref_cnt;
    struct semaphore queue_semaphore;

    spinlock_t queue_spinlock;
    int ql, qr, qsz;
    struct notification *queue;

    wait_queue_head_t wq;
};

static struct splay_tree_node *root = NULL;

static inline struct notification_queue *get_queue(struct splay_tree_node *n) {
    return container_of(n, struct notification_queue, node);
}

static struct splay_tree_node *find(void *val) {
    struct splay_tree_node *ret;

    ret = root;
    while (ret && get_queue(ret)->pid != val) {
        if (val < (void*)(get_queue(ret)->pid))
            ret = ret->L;
        else
            ret = ret->R;
    }

    if (ret) {
        root = splay(ret);
        return ret;
    }
    return NULL;
}

static struct splay_tree_node *lower_bound(struct splay_tree_node *u, void *val) {
    struct splay_tree_node *cur, *best;

    best = NULL;
    cur = u;

    while (cur) {
        if ((void*)(get_queue(cur)->pid) >= val) {
            if (!best || get_queue(cur)->pid < get_queue(best)->pid)
                best = cur;
        }
        if (val < (void*)(get_queue(cur)->pid))
            cur = cur->L;
        else
            cur = cur->R;
    }

    return best;
}

static void split(struct splay_tree_node *u, void *val, struct splay_tree_node **lt,
                  struct splay_tree_node **rt) {
    struct splay_tree_node *v;

    v = lower_bound(u, val);
    if (!v) {
        *lt = u;
        *rt = NULL;
        return;
    }
    splay(v);
    *lt = v->L;
    *rt = v;
    if (v->L)
        v->L->par = NULL;
    v->L = NULL;
}

static struct splay_tree_node *merge(struct splay_tree_node *lt, struct splay_tree_node *rt) {
    if (!lt)
        return rt;
    if (!rt)
        return lt;
    while (lt->R)
        lt = lt->R;
    lt = splay(lt);
    BUG_ON(lt->R);
    lt->R = rt;
    rt->par = lt;
    return lt;
}

static void remove_value(struct pid *pid) {
    struct notification_queue *snode;
    struct splay_tree_node *n;

    printk(KERN_INFO "sandboxer: removing pid %p\n", pid);

    n = find(pid);
    if (!n)
        return;
    splay(n);
    if (n->L)
        n->L->par = NULL;
    if (n->R)
        n->R->par = NULL;
    root = merge(n->L, n->R);
    snode = get_queue(n);
    kfree(snode->queue);
    kfree(snode);
}

struct notification_queue* allocate_queue(struct pid *pid) {
    struct notification_queue *ret;

    ret = kmalloc(sizeof(struct notification_queue), GFP_ATOMIC);
    if (!ret)
        return NULL;
    ret->queue = kmalloc(sizeof(struct notification) * notification_queue_size, GFP_ATOMIC);
    if (!(ret->queue)) {
        kfree(ret);
        return NULL;
    }
    ret->pid = pid;
    ret->ref_cnt = 0;
    sema_init(&(ret->queue_semaphore), notification_queue_size);
    spin_lock_init(&(ret->queue_spinlock));
    ret->ql = ret->qr = ret->qsz = 0;
    init_waitqueue_head(&ret->wq);
    return ret;
}

static int insert_value(struct pid *pid) {
    struct notification_queue *n;

    printk(KERN_INFO "sandboxer: inserting pid %p\n", pid); 

    if (find(pid))
        return 0;
    n = allocate_queue(pid);
    if (!n)
        return -ENOMEM;
    n->node.par = NULL;
    split(root, pid, &n->node.L, &n->node.R);
    if (n->node.L) {
        n->node.L->par = &n->node;
    }
    if (n->node.R) {
        n->node.R->par = &n->node;
    }
    root = &n->node;
    return 0;
}

static DEFINE_SPINLOCK(splay_lock);

enum notification_query_t {
    NOTIFICATION_ADD, /* send notification */
    NOTIFICATION_READ,
    NOTIFICATION_ADD_SLOT,
    NOTIFICATION_REMOVE_SLOT,
};

static bool check_queue_and_lock(struct notification_queue *q) {
    if (q->qsz != 0) {
        spin_lock(&splay_lock);
        return true;
    }
    return false;
}

static struct notification perform_query(enum notification_query_t type, int *errno, struct pid *pid, void *data) {
    struct notification ret;
    struct notification *notification;
    struct splay_tree_node *n;
    struct notification_queue *q;
    bool do_wake_up;
    bool removed_slot = false;

    spin_lock(&splay_lock);
    n = find(pid);
    spin_unlock(&splay_lock);

    if ((type == NOTIFICATION_READ || type == NOTIFICATION_ADD_SLOT) && (n == NULL)) {
        get_pid(pid);
    }

    spin_lock(&splay_lock);

    if (type == NOTIFICATION_ADD) {

        notification = data;
        n = find(pid);
        if (!n) {
            printk(KERN_ERR "sandboxer: BUG: type %d pid %p\n", notification->type, pid);
        }
        BUG_ON(!n);
        q = get_queue(n);

        spin_unlock(&splay_lock);

        down(&q->queue_semaphore);

        spin_lock(&q->queue_spinlock);
        q->queue[q->qr] = *notification;
        q->qr = (q->qr + 1) % notification_queue_size;
        do_wake_up = (++q->qsz == 1);
        spin_unlock(&q->queue_spinlock); 

        if (do_wake_up) {
            wake_up_interruptible(&q->wq);
        }

        ret = *notification;
        goto out_unlocked;

    } else if (type == NOTIFICATION_READ) {

        n = find(pid);
        if (!n) {
            *errno = insert_value(pid);
            n = find(pid);
            if (!n) {
                goto out_locked;
            }
        }

        q = get_queue(n);
        
        if (q->qsz == 0) {
            spin_unlock(&splay_lock);
            if (wait_event_interruptible(q->wq, check_queue_and_lock(q))) {
                *errno = -ERESTARTSYS;
                goto out_unlocked;
            }
        }

        spin_lock(&q->queue_spinlock);        
        ret = q->queue[q->ql];
        q->ql = (q->ql + 1) % notification_queue_size;
        q->qsz--;
        spin_unlock(&q->queue_spinlock);

        if (q->ref_cnt == 0 && q->qsz == 0) {
            remove_value(pid);
            spin_unlock(&splay_lock);
            removed_slot = true;
            goto out_unlocked;
        }

        up(&q->queue_semaphore);

    } else if (type == NOTIFICATION_ADD_SLOT) {

        n = find(pid);
        if (n) {
            get_queue(n)->ref_cnt++;
            *errno = 0;
        } else {
            *errno = insert_value(pid);
            n = find(pid);
            if (n) {
                get_queue(n)->ref_cnt++;
            }
        }
        printk(KERN_INFO "sandboxer: connected slot to mentor %p. mentor ref_cnt is now %d.\n", pid, get_queue(n)->ref_cnt);

    } else if (type == NOTIFICATION_REMOVE_SLOT) {

        n = find(pid);
        BUG_ON(!n);
        q = get_queue(n);
        BUG_ON(q->ref_cnt == 0);
        q->ref_cnt--;
        printk(KERN_INFO "sandboxer: disconnected slot from mentor %p. mentor ref_cnt is now %d.\n", pid, q->ref_cnt);
        if (q->ref_cnt == 0) {
            if (q->qsz == 0 && !waitqueue_active(&q->wq)) {
                remove_value(pid);
                removed_slot = true;
            }
        }

    }

out_locked:

    spin_unlock(&splay_lock);
    
out_unlocked:
    
    if (removed_slot)
        put_pid(pid);

    return ret;
}

int increase_mentor_refcnt(struct pid *pid) {
    int errno;
    perform_query(NOTIFICATION_ADD_SLOT, &errno, pid, NULL);
    return errno;
}

void decrease_mentor_refcnt(struct pid *pid) {
    perform_query(NOTIFICATION_REMOVE_SLOT, NULL, pid, NULL);
}

void send_notification(struct pid *pid, struct notification n) {
    perform_query(NOTIFICATION_ADD, NULL, pid, &n);
}

int read_notification(struct pid *pid, struct notification *n) {
    int errno = 0;
    *n = perform_query(NOTIFICATION_READ, &errno, pid, NULL);
    return errno;
}
