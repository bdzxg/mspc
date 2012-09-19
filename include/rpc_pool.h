
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_POOL_H_INCLUDED_
#define _RPC_POOL_H_INCLUDED_

#include "rpc_types.h"

BEGIN_DECLS

typedef void (*rpc_pool_cleanup_pt)(void *data);

typedef struct rpc_pool_cleanup_s rpc_pool_cleanup_t;

struct rpc_pool_cleanup_s {
  rpc_pool_cleanup_pt handler;
  void  *data;
  rpc_pool_cleanup_t  *next;
};

typedef struct rpc_pool_large_s rpc_pool_large_t;

struct rpc_pool_large_s {
  rpc_pool_large_t  *next;
  void  *alloc;
};

typedef struct {
  u_char  *last;
  u_char  *end;
  rpc_pool_t  *next;
  rpc_uint_t  failed;
} rpc_pool_data_t;

struct rpc_pool_s {
  rpc_pool_data_t d;
  size_t  max;
  rpc_pool_t  *current;
  rpc_pool_large_t  *large;
  rpc_pool_cleanup_t  *cleanup;
};

rpc_pool_t *rpc_pool_create(size_t size);
void rpc_pool_destroy(rpc_pool_t *pool);
void rpc_pool_reset(rpc_pool_t *pool);

void *rpc_palloc(rpc_pool_t *pool, size_t size);
void *rpc_pcalloc(rpc_pool_t *pool, size_t size);
rpc_int_t rpc_pfree(rpc_pool_t *pool, void *p);

rpc_pool_cleanup_t *rpc_pool_cleanup_add(rpc_pool_t *p, size_t size);

END_DECLS
#endif
