#ifndef PTSANDBOX_UTILS_SET_H
#define PTSANDBOX_UTILS_SET_H

#include <stddef.h>
#include "mem.h"
#include "attributes.h"

#define INTERNAL(NAME) INTERNAL_BASE(utils_set, NAME)

// Provides cartesian node definitions,
// You want to use more friendly wrappers bellow
#define DECLARE_TEMPLATED(INTERNAL(cartesian_node), TYPE) \
    struct TEMPLATED(INTERNAL(cartesian_node),TYPE) { \
        TEMPLATED(INTERNAL(cartesian_node), TYPE)* left; \
        TEMPLATED(INTERNAL(cartesian_node), TYPE)* right;
        TYPE key; \
        long prio; \
        size_t subtreesz; \
    }; \
    \
    typedef TEMPLATED(INTERNAL(cartesian_node), TYPE)* TEMPLATED(INTERNAL(p_cartesian), TYPE); \
    \
    DECLARE_TEMPLATED(allocate_objs, TEMPLATED(INTERNAL(cartesian_node),TYPE)); \
    DECLARE_TEMPLATED(deallocate_objs, TEMPLATED(INTERNAL(cartesian_node), TYPE)); \
    DECLARE_TEMPLATED2(pair, TEMPLATED(INTERNAL(p_cartesian), TYPE), int); \
    DECLARE_TEMPLATED2(make_pair, TEMPLATED(INTERNAL(p_cartesian), TYPE), int); \
    \
    TEMPLATED(INTERNAL(p_cartesian),TYPE) TEMPLATED(INTERNAL(cartesian_create), TYPE)(TYPE obj) { \
        TEMPLATED(INTERNAL(p_cartesian),TYPE) p_node = TEMPLATED(allocate_objs, TEMPLATED(INTERNAL(cartesian_node),TYPE))(1); \
        p_node->key = obj; \
        p_node->prio = rand(); /* TODO: DON'T USE LIBC RANDOM, IT IS KNOWN TO BE POOR.*/ \
        p_node->left = p_node->right = NULL; \
        p_node->subtreesz = 1; \
        return p_node; \
    } \
    \
    void TEMPLATED(INTERNAL(cartesian_destroy), TYPE)(TEMPLATED(INTERNAL(p_cartesian),TYPE) p_node) { \
        if (p_node->left != NULL) \
            TEMPLATED(INTERNAL(cartesian_destroy), TYPE)(p_node->left); \
        if (p_node->right != NULL) \
            TEMPLATED(INTERNAL(cartesian_destroy),TYPE)(p_node->right); \
        TEMPLATED(deallocate_objs,TEMPLATED(INTERNAL(cartesian_node),TYPE))(p_node, 1); \
    } \
    \
    void TEMPLATED(INTERNAL(cartesian_recalc), TYPE)(TEMPLATED(INTERNAL(p_cartesian), TYPE) p_node) { \
        if (p_node != NULL) { \
            p_node->subtreesz = 1; \
            if (p_node->left != NULL) \
                p_node->subtreesz += p_node->left->subtreesz; \
            if (p_node->right != NULL) \
                p_node->subtreesz += p_node->right->subtreesz; \
        } \
    } \
    \
    TEMPLATED(INTERNAL(p_cartesian), TYPE) TEMPLATED(INTERNAL(cartesian_merge, TYPE))(TEMPLATED(INTERNAL(p_cartesian, TYPE)) l, TEMPLATED(INTERNAL(p_cartesian), TYPE) r) { \
        if (l == NULL || r == NULL) \
            return (l != NULL ? l : r); \
        else if (l->prio > r->prio) { \
            l->right = TEMPLATED(INTERNAL(cartesian_merge), TYPE)(l->right, r); \
            TEMPLATED(INTERNAL(cartesian_recalc), TYPE)(l); \
            return l; \
        } else { \
            r->left = TEMPLATED(INTERNAL(cartesian_merge), TYPE)(l, r->left); \
            TEMPLATED(INTERNAL(cartesian_recalc), TYPE)(r);                   \
            return r; \
        } \
    } \
    \
    TEMPLATED2(pair, TEMPLATED(INTERNAL(p_cartesian),TYPE), int) TEMPLATED(INTERNAL(cartesian_remove), TYPE)(TEMPLATED(INTERNAL(p_set_node), TYPE) root, const TYPE* obj) { \
        if (root == NULL) \
            return TEMPLATED2(make_pair, TEMPLATED(INTERNAL(p_cartesian), TYPE), int)(root, 0); \
        else {\
            int res = TEMPLATED(compare, TYPE)(&(root->obj), obj); \
            if (res == 0) { \
                TEMPLATED(INTERNAL(p_cartesian), TYPE) l = root->left; \
                TEMPLATED(INTERNAL(p_cartesian), TYPE) r = root->right; \
                root->left = root->right = NULL; \
                TEMPLATED(INTERNAL(cartesian_destroy), TYPE)(root); \
                return TEMPLATED2(make_pair, TEMPLATED(INTERNAL(p_cartesian), TYPE), int)(TEMPLATED(INTERNAL(cartesian_merge), TYPE)(l, r), 1); \
            } else { \
                TEMPLATED2(pair, TEMPLATED(INTERNAL(p_cartesian), TYPE), int) res; \
                if (res > 0) { \
                    /* value in our node too big, try less */   \
                    res = TEMPLATED(INTERNAL(cartesian_remove), TYPE)(root->left, obj); \
                    root->left = res.first; \
                } else { \
                    res = TEMPLATED(INTERNAL(cartesian_remove), TYPE)(root->right, obj); \
                    root->right = res.first; \
                } \
                TEMPLATED(INTERNAL(cartesian_recalc), TYPE)(root);
                return TEMPLATED2(make_pair, TEMPLATED(INTERNAL(p_cartesian), TYPE), int)(root, 1); \
            } \
        } \
    } \     
    \
    TYPE* TEMPLATED(INTERNAL(cartesian_find), TYPE)(TEMPLATED(INTERNAL(p_cartesian), TYPE) root, const TYPE* obj) { \
        if (root == NULL) \
            return NULL; \
        int res = TEMPLATED(compare, TYPE)(&(root->obj), obj); \
        if (res == 0) \
            return &(root->obj); \
        else if (res < 0) \
            /* Our value is too small, try bigger */ \
            return TEMPLATED(INTERNAL(cartesian_find), TYPE)(root->right, obj); \
        else \
            return TEMPLATED(INTERNAL(cartesian_find), TYPE)(root->left, obj); \
    } \
    \
    /* Insert new value in cartesian, assumes, that supplied element has subtree of exactly =1 element */ \
    TEMPLATED(INTERNAL(p_cartesian), TYPE) TEMPLATED(INTERNAL(cartesian_insert), TYPE)(TEMPLATED(INTERNAL(p_cartesian), TYPE) main, TEMPLATED(INTERNAL(p_cartesian, TYPE)) new_elem) {\
        if (main == NULL) \
            return new_elem; \
        if (new_elem->prio >= main->prio) { \
            
            

// using guide:
// Call this macros somewhere in your sources to instantinate required associative_array.
// To construct templated name use macros above.
//
// also you need to implement function:
// int TEMPLATED(compare,TYPE)(const TYPE* obj1, const TYPE* obj2);
//
// it will be used to compare elements.
#define DECLARE_SET(TYPE) \
    struct TEMPLATED(set_node,TYPE) { \
        TEMPLATED(set_node,TYPE) * left; \
        TEMPLATED(set_node,TYPE) * right; \
        TYPE key; \
        long prio; \
        size_t subtreesz; \
    }; \
    \
    struct TEMPLATED(set,TYPE) { \
        TEMPLATED(set,TYPE)* root; \
    }; \
    \
    \
    TEMPLATED(set,TYPE)* TEMPLATED(set_create,TYPE)(void) { \
        TEMPLATED(set,TYPE)* res = TEMPLATED(allocate_objs,TEMPLATED(set,TYPE))(1); \
        res->root = NULL; \
        return res; \
    } \
    \
    void TEMPLATED(set_destroy,TYPE)(TEMPLATED(set,TYPE)* p_set) { \
        if (p_set != NULL) { \
            if (p_set->root != NULL) \
                TEMPLATED(set_node_destroy,TYPE)(p_set->root); \
            TEMPLATED(deallocate_objs,TEMPLATED(set,TYPE))(p_set,1);    \
        } \
    } \
    \



#undef INTERNAL

#endif
