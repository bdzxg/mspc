
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_RBTREE_H_INCLUDED_
#define _RPC_RBTREE_H_INCLUDED_

#include "rpc_types.h"

typedef rpc_uint_t  rpc_rbtree_key_t;
typedef rpc_int_t   rpc_rbtree_key_int_t;

typedef struct rpc_rbtree_node_s  rpc_rbtree_node_t;

struct rpc_rbtree_node_s {
  rpc_rbtree_key_t  key;
  rpc_rbtree_node_t *left;
  rpc_rbtree_node_t *right;
  rpc_rbtree_node_t *parent;
  u_char  color;
  u_char  data;
};

typedef struct rpc_rbtree_s rpc_rbtree_t;

typedef void (*rpc_rbtree_insert_pt) (rpc_rbtree_node_t *root,
    rpc_rbtree_node_t *node,  rpc_rbtree_node_t *sentinel);

struct rpc_rbtree_s {
  rpc_rbtree_node_t *root;
  rpc_rbtree_node_t *sentinel;
  rpc_rbtree_insert_pt  insert;
};

#define rpc_rbtree_init(tree, s, i) \
  rpc_rbtree_sentinel_init(s);\
  (tree)->root = s; \
  (tree)->sentinel = s; \
  (tree)->insert = i 

void rpc_rbtree_insert(rpc_rbtree_t *tree,
    rpc_rbtree_node_t *node);
void rpc_rbtree_delete(rpc_rbtree_t *tree,
    rpc_rbtree_node_t *node);
void rpc_rbtree_insert_value(rpc_rbtree_node_t *root, rpc_rbtree_node_t *node,
    rpc_rbtree_node_t *sentinel);
void rpc_rbtree_insert_timer_value(rpc_rbtree_node_t *root, rpc_rbtree_node_t *node,
    rpc_rbtree_node_t *sentinel);

#define rpc_rbt_red(node) ((node)->color = 1)
#define rpc_rbt_black(node) ((node)->color = 0)
#define rpc_rbt_is_red(node)  ((node)->color)
#define rpc_rbt_is_black(node)  (!rpc_rbt_is_red(node))
#define rpc_rbt_copy_color(n1, n2)  (n1->color = n2->color)

#define rpc_rbtree_sentinel_init(node) rpc_rbt_black(node);

static inline rpc_rbtree_node_t *
rpc_rbtree_min(rpc_rbtree_node_t *node, rpc_rbtree_node_t *sentinel)
{
  while (node->left != sentinel) {
    node = node->left;
  }

  return node;
}

#endif
