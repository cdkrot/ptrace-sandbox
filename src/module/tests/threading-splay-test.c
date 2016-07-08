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

#include "test-splay.h"
#include "../splay.h"
#include "../hashmap.h"

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

// Related constants:

const int HASHMAP_SIZE = 4096;

// Code related to the tree:

struct node {
    int x;
    struct splay_tree_node node;
};

static struct splay_tree_node *root = NULL;

static inline struct node *get_set_node(struct splay_tree_node *n) {
    return container_of(n, struct node, node);
}

static struct splay_tree_node *find(int val) {
    struct splay_tree_node *ret;

    ret = root;
    while (ret && get_set_node(ret)->x != val) {
        if (val < get_set_node(ret)->x)
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

/* This implementation is recursive. We certainly cannot use it inside the kernel.

static struct splay_tree_node *lower_bound(struct splay_tree_node *u, int val) {
    struct splay_tree_node *n;

    if (!u)
       return NULL;
    if (get_set_node(u)->x > val) {
        if (u->L)
            n = lower_bound(u->L, val);
        else
            return u;
    } else {
        if (u->R)
            n = lower_bound(u->R, val);
        else
            return NULL;
    }
    if (!n) {
        if (get_set_node(u)->x >= val)
            return u;
        else
            return NULL;
    } else {
        if (get_set_node(u)->x < val)
            return n;
        else if (get_set_node(u)->x < get_set_node(n)->x)
            return u;
        else
            return n;
    }
}
*/

static struct splay_tree_node *lower_bound(struct splay_tree_node *u, int val) {
    struct splay_tree_node *cur, *best;

    best = NULL;
    cur = u;

    while (cur) {
        if (get_set_node(cur)->x >= val) {
            if (!best || get_set_node(cur)->x < get_set_node(best)->x)
                best = cur;
        }
        if (val < get_set_node(cur)->x)
            cur = cur->L;
        else
            cur = cur->R;
    }

    return best;
}

static void split(struct splay_tree_node *u, int val, struct splay_tree_node **lt,
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

static void remove_value(int x) {
    struct node *snode;
    struct splay_tree_node *n;

    n = find(x);
    if (!n)
        return;
    splay(n);
    if (n->L)
        n->L->par = NULL;
    if (n->R)
        n->R->par = NULL;
    root = merge(n->L, n->R);
    snode = get_set_node(n);
    kfree(snode);
}

static int insert_value(int x) {
    struct node *n;

    if (find(x))
        return 0;
    n = kmalloc(sizeof(struct node), GFP_ATOMIC);
    if (!n)
        return -ENOMEM;
    n->x = x;
    n->node.par = NULL;
    split(root, x, &n->node.L, &n->node.R);
    if (n->node.L) {
        n->node.L->par = &n->node;
    }
    if (n->node.R) {
        n->node.R->par = &n->node;
    }
    root = &n->node;
    return 0;
}

DEFINE_SPINLOCK(splay_lock);

struct hashmap hmp;

int perform_query(int type, int q) {
    int errno, ans;

    spin_lock(&splay_lock);

    errno = 0;
    if (type == 0)
        errno = insert_value(q);
    else if (type == 1)
        remove_value(q);
    else {
        ans = (find(q) != NULL) + 1;
        hashmap_set(&hmp, (void*)((size_t)current->pid), (void*)((size_t)ans), NULL);
    }

    spin_unlock(&splay_lock);

    return errno;
}

static void free_tree(void) {
/*  This version can also be written but it works in O(n log n) time:
    
    while (root) {
        perform_query(1, get_set_node(root)->x);
    }
*/
    struct splay_tree_node *cur, *rem;
    cur = root;
    while (cur) {
        if (cur->L) {
            cur = cur->L;
        } else if (cur->R) {
            cur = cur->R;
        } else {
            rem = cur->par;
            if (rem) {
                if (rem->L == cur)
                    rem->L = NULL;
                else
                    rem->R = NULL;
            }
            kfree(get_set_node(cur));
            cur = rem;
        }
    }
}

// Code related to /proc/sbsplay_test file:

static size_t hash_trivial(const struct hashmap *_hmp, const void *obj, size_t tryid) {
    size_t thehash = (size_t)obj;
    thehash %= _hmp->size;
    thehash += tryid * tryid;
    thehash %= _hmp->size;

    return thehash;
}

static bool equal_trivial(const void *obj1, const void *obj2) {
    return obj1 == obj2;
}

static void *seq_start(struct seq_file *s, loff_t *pos) {
    if (*pos == 0 && hashmap_get(&hmp, (void*)((size_t)current->pid)) != (void*)(NULL)) {
        return hashmap_get(&hmp, (void*)((size_t)current->pid));
    } else {
        *pos = 0;
        return NULL;
    }
}

static int seq_show(struct seq_file *s, void *v) {
    seq_printf(s, "%lu\n", (size_t)v);
    return 0;
}

static void* seq_next(struct seq_file *s, void *v, loff_t *pos) {
    ++*pos;
    return NULL;
}

static void seq_stop(struct seq_file *s, void *v) {}

static const struct seq_operations seq_ops = {
    .start = seq_start,
    .next = seq_next,
    .stop = seq_stop,
    .show = seq_show
};

int parse_int(const char* buffer, size_t length) {
    int ans = 0, i = 0;
    for (i = 0; i < length; i++) {
        ans *= 10;
        ans += buffer[i] - '0';
    }
    return ans;
}

static ssize_t write(struct file *_file, const char *buffer, size_t length, loff_t *offset) {
    int qtype, qint, errno;

    errno = 0;
    if (*offset > 0)
        return 0;
    qint = parse_int(buffer + 2, length - 2);
    if (buffer[0] == '+')
        qtype = 0;
    else if (buffer[0] == '-')
        qtype = 1;
    else 
        qtype = 2;

//    printk("sandboxer: write qtype=%d qint=%d\n", qtype, qint);

    if ((errno = perform_query(qtype, qint)) != 0)
        return errno;

/*    if (qtype == 2) {
        printk("sandboxer: query answer is %d\n", (int)(size_t)hashmap_get(&hmp, (void*)((size_t)current->pid)));
    }*/

    *offset = length;
    return length;
}

static int open(struct inode *_inode, struct file *_file) {
    return seq_open(_file, &seq_ops);
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
    .write = write
};

static struct proc_dir_entry *entry;

int init_splay_threading_test(int initlib_mode, __attribute__((unused)) void *ignored) {
    int errno;

    if (initlib_mode == 1) {
        errno = 0;
        hmp.hash = hash_trivial;
        hmp.equal = equal_trivial;
        hmp.tolerance = 10;
        if ((errno = init_hashmap(&hmp, HASHMAP_SIZE)) != 0) {
            printk(KERN_INFO "sandboxer: hashmap initialization failed while "
                             "initializing splay threading test\n");
            goto out;
        }
        entry = proc_create("sbsplay_test", 0666, NULL, &fops);
        if (!entry) {
            errno = -ENOMEM;
            goto out_hmp_free;
        }
        
        root = NULL;

        goto out;
    } else {
        free_tree();
        errno = 0;
        goto out_entry_free;
    }

    out_entry_free:
    remove_proc_entry("sbsplay_test", NULL);

    out_hmp_free:
    hashmap_free(&hmp);

    out:
    return errno;
}
