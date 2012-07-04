/*
 * rpc_sessionpool.h
 *
 *  Created on: 2012-2-29
 *      Author: yanghu
 */

#ifndef RPC_SESSIONPOOL_H_
#define RPC_SESSIONPOOL_H_

#include "rpc_types.h"

BEGIN_DECLS

#define RPC_POOL_SIZE 1024

#define rpc_sessionpool_new() rpc_sessionpool_new_size(RPC_POOL_SIZE)

rpc_sessionpool_t* rpc_sessionpool_new_size(size_t size);

int rpc_sessionpool_insert(rpc_sessionpool_t *pool, void *data);

void *rpc_sessionpool_get(rpc_sessionpool_t *pool, int index);

void *rpc_sessionpool_remove(rpc_sessionpool_t *pool, int index);

void rpc_sessionpool_free(rpc_sessionpool_t *pool);

size_t rpc_sessionpool_size(rpc_sessionpool_t *pool);

size_t rpc_sessionpool_capacity(rpc_sessionpool_t *pool);
END_DECLS

#endif /* RPC_SESSIONPOOL_H_ */
