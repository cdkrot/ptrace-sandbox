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

#include <linux/bug.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>

static int last, step;
static const int MOD = 1791791;

static const int ELEMS = 1e5, OPS = 2e6;

static int next_rand(void) {
    return last = (last + step) % MOD;
}

struct map_node {
    int key, value;
    struct splay_tree_node node;
};

static struct splay_tree_node *root = NULL;

static inline struct map_node *get_map_node(struct splay_tree_node *n) {
    return container_of(n, struct map_node, node);
}

static struct splay_tree_node *find_node_rec(struct splay_tree_node *u, int val) {
    if (!u)
        return NULL;
    else if (get_map_node(u)->key == val)
        return u;
    else if (get_map_node(u)->key < val)
        return find_node_rec(u->R, val);
    else
        return find_node_rec(u->L, val);
}

static struct splay_tree_node *find(struct splay_tree_node *u, int val) {
    struct splay_tree_node *n;

    n = find_node_rec(u, val);
    if (n)
        root = splay(n);
    return n;
}

static struct splay_tree_node *lower_bound(struct splay_tree_node *u, int val) {
    struct splay_tree_node *n;

    if (!u)
        return NULL;
    if (get_map_node(u)->key > val) {
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
        if (get_map_node(u)->key >= val)
            return u;
        else
            return NULL;
    } else {
        if (get_map_node(u)->key < val)
            return n;
        else if (get_map_node(u)->key < get_map_node(n)->key)
            return u;
        else
            return n;
    }
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

static int set_val(int key, int val) {
    struct map_node *cr;

    if (find(root, key)) {
        get_map_node(root)->value = val;
    } else {
        cr = kmalloc(sizeof(struct map_node), GFP_KERNEL);
        if (!cr)
            return -ENOMEM;
        cr->key = key;
        cr->value = val;
        cr->node.par = NULL;
        split(root, key, &cr->node.L, &cr->node.R);
        if (cr->node.L) {
            cr->node.L->par = &cr->node;
        }
        if (cr->node.R) {
            cr->node.R->par = &cr->node;
        }
        root = &cr->node;
    }

    return 0;
}

static int get_val(int key) {
    if (find(root, key))
        return get_map_node(root)->value;
    else
        return -1;
}

static void free_tree(struct splay_tree_node *n) {
    if (!n)
        return;
    free_tree(n->L);
    free_tree(n->R);
    kfree(get_map_node(n));
}

#define RANDOM_SPLAY_TEST_EXPECTED(b, msg)    \
    if (!(b)) {                               \
        printk(KERN_INFO "sandboxer: " msg);  \
        return -EFAULT;                       \
    }

int random_splay_test(int initlib_mode, __attribute__((unused)) void* ignored) {
    int type, key, val, i, *arr;

    if (initlib_mode != 1)
        return 0;

    // initialize random number generator
    last = 179;
    step = current_kernel_time().tv_nsec % MOD;
    printk(KERN_INFO "sandboxer: random_splay_test: step = %d\n", step);

    // initialize test array
    arr = kmalloc(sizeof(int) * ELEMS, GFP_KERNEL);
    if (!arr) {
        printk(KERN_INFO "sandboxer: memory allocation failed in random splay tree test\n");
        return -ENOMEM;
    }
    for (i = 0; i < ELEMS; i++)
        arr[i] = -1;

    // do some random tests
    for (i = 0; i < OPS; i++) {
        type = next_rand() % 2;
        if (type == 0) {
            key = next_rand() % ELEMS;
            RANDOM_SPLAY_TEST_EXPECTED(arr[key] == get_val(key), "random splay tree test failed\n")
        } else {
            key = next_rand() % ELEMS;
            val = next_rand();
            RANDOM_SPLAY_TEST_EXPECTED(set_val(key, val) == 0, "memory allocation failed in random splay tree test\n");
            arr[key] = val;
        }
    }

    kfree(arr);
    free_tree(root);

    return 0;
}
