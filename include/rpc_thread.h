/*
 * rpc_thread.h
 *
 *  Created on: 2012-2-22
 *      Author: yanghu
 */

#ifndef RPC_THREAD_H_
#define RPC_THREAD_H_

#include "rpc_types.h"
BEGIN_DECLS

typedef struct rpc_thread_notify_s rpc_thread_notify_t;

struct rpc_thread_dispatch_s {
	pthread_t thread_id;

	struct ev_loop *loop;

	rpc_server_t *svr;
};

struct rpc_thread_notify_s {
  int fd;

  struct sockaddr *sockaddr;
  socklen_t socklen;

  void *owner;
};

struct rpc_thread_worker_s {
	pthread_t thread_id;

	struct ev_loop *loop;
  struct ev_async notify;

	rpc_queue_t *q;
	void *data;

};

rpc_int_t rpc_dispatch_init(rpc_thread_dispatch_t *th, struct ev_loop *l);

rpc_int_t rpc_dispatch_start(rpc_thread_dispatch_t *th);

void rpc_sworker_init(rpc_thread_worker_t *th);

void rpc_sworker_start(rpc_thread_worker_t *th);

void rpc_cworker_init(rpc_thread_worker_t *th);

void rpc_cworker_start(rpc_thread_worker_t *th);


END_DECLS
#endif /* RPC_THREAD_H_ */
