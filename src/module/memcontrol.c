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

#include <linux/moduleparam.h>

#include "memcontrol.h"
#include "splay.h"

int debug_memcontrol = 0;
module_param(debug_memcontrol, int, 0000);

struct node {
    void *address;
    size_t size;
    enum sandboxer_malloc_target sbmt;
    struct splay_tree_node node;
};

static struct splay_tree_node *root = NULL;

static inline struct node *get_set_node(struct splay_tree_node *n) {
    return container_of(n, struct node, node);
}

static struct splay_tree_node *find(const void *val) {
    struct splay_tree_node *ret;

    ret = root;
    while (ret && get_set_node(ret)->address != val) {
        if (val < get_set_node(ret)->address)
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

static struct splay_tree_node *lower_bound(struct splay_tree_node *u, const void *val) {
    struct splay_tree_node *cur, *best;

    best = NULL;
    cur = u;

    while (cur) {
        if (get_set_node(cur)->address >= val) {
            if (!best || get_set_node(cur)->address < get_set_node(best)->address)
                best = cur;
        }
        if (val < get_set_node(cur)->address)
            cur = cur->L;
        else
            cur = cur->R;
    }

    return best;
}

static void split(struct splay_tree_node *u, const void *val, struct splay_tree_node **lt,
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

static void remove_value(const void *x) {
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
}

static int insert_value(struct node *n) {
    n->node.par = NULL;
    split(root, n->address, &n->node.L, &n->node.R);
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

static void free_tree(void) {
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

#ifdef CONFIG_64BIT
typedef atomic64_t atomic_size_t;
#define atomic_size_t_op(op, ...) atomic64_##op(__VA_ARGS__)
#else
typedef atomic_t atomic_size_t;
#define atomic_size_t_op(op, ...) atomic_##op(__VA_ARGS__)
#endif

struct memstats {
    atomic_t alloc;
    atomic_t freed;
    atomic_size_t waste;
}; 

static struct memstats stats[SBMT_NUM];

void *sb_kmalloc(size_t size, gfp_t flags, enum sandboxer_malloc_target sbmt) {
    void *res;
    struct node *n;

    if (!debug_memcontrol)
        return kmalloc(size, flags);
    else {
        res = kmalloc(size, flags);

        if (!res)
            return NULL;

        n = kmalloc(sizeof(struct node), flags);
        if (!n) {
            kfree(res);
            return NULL;
        }
        n->address = res;
        n->size = size;
        n->sbmt = sbmt;

        // TODO: Is this really safe?
        spin_lock(&splay_lock);
        insert_value(n);
        spin_unlock(&splay_lock);
        
        atomic_inc(&(stats[sbmt].alloc));
        atomic_size_t_op(add_return, size, &(stats[sbmt].waste));
        return res;
    }
}

void sb_kfree(const void *v) {
    struct splay_tree_node *n;
    struct node *sn;
    enum sandboxer_malloc_target target;
    size_t sz;

    if (!debug_memcontrol)
        kfree(v);
    else {
        if (!v)
            return;
        spin_lock(&splay_lock);
        n = find(v);
        BUG_ON(!n);
        sn = get_set_node(n);
        target = sn->sbmt;
        sz = sn->size;
        remove_value(v);
        kfree(sn);
        spin_unlock(&splay_lock);

        kfree(v);

        atomic_inc(&(stats[target].freed));
        atomic_size_t_op(sub_return, sz, &(stats[target].waste));
        
    }
}
    
#define PRINT_STATS(module, sbmt) \
    printk(KERN_INFO "sandboxer: (" module ") allocated %d pieces, freed %d pieces, wasted %ld bytes\n", \
    atomic_read(&(stats[sbmt].alloc)), \
    atomic_read(&(stats[sbmt].freed)), \
    atomic_size_t_op(read, &(stats[sbmt]).waste));
    

int init_or_shutdown_memcontrol(int initlib_mode, __attribute__((unused)) void *ignored) {
    int i;

    if (initlib_mode) {
        if (debug_memcontrol) {
            for (i = 0; i < SBMT_NUM; i++) {
                atomic_set(&(stats[i].alloc), 0);
                atomic_set(&(stats[i].freed), 0);
                atomic_size_t_op(set, &(stats[i].waste), 0);
            }
        }
    } else {
        if (debug_memcontrol) {
            printk(KERN_INFO "sandboxer: start of debug_memcontrol stats\n");
            PRINT_STATS("slots", SBMT_SLOT)
            PRINT_STATS("hashmap", SBMT_HASHMAP)
            PRINT_STATS("notification structs", SBMT_NOTIFICATIONS_STRUCT)
            PRINT_STATS("notifications queues", SBMT_NOTIFICATION_QUEUE)
            PRINT_STATS("notifications", SBMT_NOTIFICATION)
            PRINT_STATS("properties", SBMT_PROPERTY)
            PRINT_STATS("property files", SBMT_PROPERTY_FILE)
            printk(KERN_INFO "sandboxer: end of debug_memcontrol stats\n");
            
            free_tree();
        }
    }
    return 0;
}
