
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_HASH_H_INCLUDED_
#define _RPC_HASH_H_INCLUDED_

#include "rpc_types.h"

typedef struct rpc_hash_s rpc_hash_t;

typedef unsigned int  (*rpc_hash_func)(const void *key);

typedef int (*rpc_equal_func)(const void *a, const void *b);

#define rpc_hash_new() rpc_hash_new_func(rpc_str_hash, rpc_str_equal)

rpc_hash_t *rpc_hash_new_func(rpc_hash_func hf, rpc_equal_func ef);

void rpc_hash_insert(rpc_hash_t *hash, void *key, void *value);

void *rpc_hash_lookup(rpc_hash_t *hash, const void *key);

rpc_int_t rpc_hash_remove(rpc_hash_t *hash, const void *key);

void rpc_hash_free(rpc_hash_t *hash);

int rpc_str_equal(const void *v1, const void *v2);

unsigned int rpc_str_hash(const void *v);

#endif
