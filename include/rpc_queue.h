/*
 * rpc_queue.h
 *
 *  Created on: 2011-3-22
 *      Author: yanghu
 */

#ifndef RPC_QUEUE_H_
#define RPC_QUEUE_H_

#include "rpc_types.h"

BEGIN_DECLS

struct rpc_queue_item_s {
	void *data;
	struct rpc_queue_item_s *next;
};

struct rpc_queue_s {
	rpc_queue_item_t *head;
	rpc_queue_item_t *tail;
  size_t count;
#ifdef __THREAD
	pthread_mutex_t mutex;
#endif
};

rpc_queue_item_t *rpc_queue_item_new();

void rpc_queue_item_free(rpc_queue_item_t *item);

rpc_queue_t* rpc_queue_new();

void rpc_queue_free(rpc_queue_t *q);

#ifdef __THREAD
  #define rpc_queue_lock(q) pthread_mutex_lock(&q->mutex)
#else
  #define rpc_queue_lock(q)
#endif

#ifdef __THREAD
  #define rpc_queue_unlock(q) pthread_mutex_unlock(&q->mutex)
#else
  #define rpc_queue_unlock(q)
#endif

void rpc_queue_push(rpc_queue_t *q, void *data);

void rpc_queue_push_unlock(rpc_queue_t *q, void *data);

void* rpc_queue_pop(rpc_queue_t *q);

void* rpc_queue_pop_unlock(rpc_queue_t *q);

#define rpc_queue_size(q) q->count

END_DECLS
#endif /* RPC_QUEUE_H_ */
