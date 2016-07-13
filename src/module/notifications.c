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
    ret->ref_cnt = (pid_task(pid, PIDTYPE_PID) != NULL);
    sema_init(&(ret->queue_semaphore), notification_queue_size);
    spin_lock_init(&(ret->queue_spinlock));
    ret->ql = ret->qr = ret->qsz = 0;
    init_waitqueue_head(&ret->wq);
    return ret;
}

static int insert_value(struct pid *pid) {
    struct notification_queue *n;

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

static bool check_queue_and_lock(struct notification_queue *q) {
    bool res;
    spin_lock(&q->queue_spinlock);
    res = (q->qsz != 0);
    if (!res)
        spin_unlock(&q->queue_spinlock);

    return res;
}

int increase_mentor_refcnt(struct pid *pid) {
    struct splay_tree_node* node;
    int errno = 0;

    spin_lock(&splay_lock);
    node = find(pid);
    if (!node) {
        if ((errno = insert_value(pid))) {
            spin_unlock(&splay_lock);
            return errno;
        }
        node = find(pid);
        BUG_ON(!node);
    }
    get_queue(node)->ref_cnt += 1;
    spin_unlock(&splay_lock);
    return 0;
}

void decrease_mentor_refcnt(struct pid *pid) {
    struct splay_tree_node* node;
    struct notification_queue* queue;

    spin_lock(&splay_lock);
    node = find(pid);
    if (!node) {
        printk("WTF\n");
        return;
    }
    BUG_ON(!node);
    queue = get_queue(node);
    BUG_ON(queue->ref_cnt == 0);
    queue->ref_cnt -= 1;
    if (queue->ref_cnt == 0)
        remove_value(pid);
    spin_unlock(&splay_lock);
}

void send_notification(struct pid *pid, struct notification notif) {
    struct splay_tree_node* node;
    struct notification_queue* queue;
    bool do_wakeup;

    spin_lock(&splay_lock);
    node = find(pid);
    BUG_ON(!node);
    spin_unlock(&splay_lock);

    queue = get_queue(node);
    down(&queue->queue_semaphore);

    spin_lock(&queue->queue_spinlock);
    queue->queue[queue->qr] = notif;
    queue->qr = (queue->qr + 1) % notification_queue_size;
    queue->qsz += 1;
    do_wakeup = (queue->qsz == 1);
    spin_unlock(&queue->queue_spinlock);

    if (do_wakeup)
        wake_up_interruptible(&queue->wq);
}

int read_notification(struct pid *pid, struct notification *out) {
    // TODO: rewrite in such manner, that errno will not be possible.
    struct splay_tree_node* node;
    struct notification_queue* queue;
    int errno = 0;

    spin_lock(&splay_lock);
    node = find(pid);
    if (!node) {
        if ((errno = insert_value(pid))) {
            spin_unlock(&splay_lock);
            return errno;
        }
        node = find(pid);
    }
    spin_unlock(&splay_lock);
    queue = get_queue(node);

    if (wait_event_interruptible(queue->wq, check_queue_and_lock(queue)))
        return -ERESTARTSYS;
    *out = queue->queue[queue->ql];
    queue->ql += 1;
    queue->qsz -= 1;
    spin_unlock(&queue->queue_spinlock);

    up(&queue->queue_semaphore);

    return 0;
}

void on_mentor_died(struct pid* pid) {
    // TODO: also set mentor_is_dead flag here.

    struct splay_tree_node* node;
    struct notification_queue* queue;
    bool do_delete;

    spin_lock(&splay_lock);
    node = find(pid);
    if (node) {
        queue = get_queue(node);
        spin_lock(&queue->queue_spinlock);
        queue->ref_cnt -= 1;
        do_delete = (queue->ref_cnt == 0);
        spin_unlock(&queue->queue_spinlock);

        if (do_delete)
            remove_value(pid);
    }
    spin_unlock(&splay_lock);
}
