
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_BUFFER_H_INCLUDED_
#define _RPC_BUFFER_H_INCLUDED_

#include "rpc_types.h"
#include "rpc_pool.h"

struct rpc_buf_s {
  u_char  *pos;
  u_char  *last;
  u_char  *start;
  u_char  *end;
  size_t size;

  rpc_pool_t *pool;
};

#define rpc_buf_alloc(pool) rpc_palloc(pool, sizeof(rpc_buf_t))
#define rpc_buf_calloc(pool) rpc_pcalloc(pool, sizeof(rpc_buf_t))

rpc_buf_t *rpc_buf_create(rpc_pool_t *pool,size_t size);

/* buf reset */
rpc_int_t rpc_buf_reset(rpc_buf_t *b);

/* buf chain */
rpc_buf_t *rpc_buf_expand(rpc_buf_t *b);
#endif
