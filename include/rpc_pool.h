
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_POOL_H_INCLUDED_
#define _RPC_POOL_H_INCLUDED_

#include "rpc_types.h"

extern rpc_uint_t rpc_pagesize;
extern rpc_uint_t rpc_pagesize_shift;
extern rpc_uint_t rpc_cacheline_size;

#define RPC_POOL_ALIGNMENT  16

#define RPC_DEFAULT_POOL_SIZE (16 * 1024)

#define RPC_MAX_ALLOC_FROM_POOL (rpc_pagesize - 1)

#define RPC_MIN_POOL_SIZE \
  rpc_align((sizeof(rpc_pool_t) + 2 * sizeof(rpc_pool_large_t)), \
      RPC_POOL_ALIGNMENT)

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

void *rpc_memalign(size_t size, size_t alignment);

rpc_pool_t *rpc_pool_create(size_t size);
void rpc_pool_destory(rpc_pool_t *pool);
void rpc_pool_reset(rpc_pool_t *pool);

void *rpc_palloc(rpc_pool_t *pool, size_t size);
void *rpc_pnalloc(rpc_pool_t *pool, size_t size);
void *rpc_pcalloc(rpc_pool_t *pool, size_t size);
void *rpc_pmemalign(rpc_pool_t *pool, size_t size, size_t alignment);
rpc_int_t rpc_pfree(rpc_pool_t *pool, void *p);

rpc_pool_cleanup_t *rpc_pool_cleanup_add(rpc_pool_t *p, size_t size);

#endif
