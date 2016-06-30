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

#include "splay.h"
#include <linux/bug.h>

enum SPLAY_TREE_CHILD {
    SPLAY_TREE_CHILD_LEFT, 
    SPLAY_TREE_CHILD_RIGHT
};

/* This function returns SPLAY_TREE_CHILD_LEFT if v is left child of its parent and SPLAY_TREE_CHILD_RIGHT if
 * v is right child of its parent. v should not be the root.
 */

static enum SPLAY_TREE_CHILD which_child(struct splay_tree_node *v) {
    BUG_ON(!v);
    BUG_ON(!(v->par));

    if (v->par->L == v)
        return SPLAY_TREE_CHILD_LEFT;
    else
        return SPLAY_TREE_CHILD_RIGHT;
}

static void left_rotate(struct splay_tree_node *v) {
    struct splay_tree_node *u;

    BUG_ON(!v);
    BUG_ON(!(v->R));
    u = v->R;
    v->R = u->L;
    if (v->R)
        v->R->par = v;
    u->par = v->par;
    if (u->par) {
        if (which_child(v) == SPLAY_TREE_CHILD_LEFT)
            u->par->L = u;
        else
            u->par->R = u;
    }
    v->par = u;
    u->L = v;
}

static void right_rotate(struct splay_tree_node *v) {
    struct splay_tree_node *u;

    BUG_ON(!v);
    BUG_ON(!(v->L));
    u = v->L;
    v->L = u->R;
    if (v->L)
        v->L->par = v;
    u->par = v->par;
    if (u->par) {
        if (which_child(v) == SPLAY_TREE_CHILD_LEFT)
            u->par->L = u;
        else
            u->par->R = u;
    }
    v->par = u;
    u->R = v;
}

/**
  * This functions rotates the edge (v->par, v). v should NOT be the root!
  */

static void rotate_edge(struct splay_tree_node *v) {
    BUG_ON(!v);
    BUG_ON(!(v->par));

    if (which_child(v) == SPLAY_TREE_CHILD_LEFT)
        right_rotate(v->par);
    else
        left_rotate(v->par);
}

struct splay_tree_node *splay(struct splay_tree_node *v) {
    BUG_ON(!v);

    while (v->par) {
        if (!(v->par->par)) {
            // zig
            rotate_edge(v);
        } else if (which_child(v) == which_child(v->par)) {
            // zig-zig
            rotate_edge(v->par);
            rotate_edge(v);
        } else {
            // zig-zag
            rotate_edge(v);
            rotate_edge(v);
        }
    }
    return v;
}
