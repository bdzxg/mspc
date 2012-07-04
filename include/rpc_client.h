/*
 * rpc_client.h
 *
 *  Created on: 2012-2-21
 *      Author: yanghu
 */

#ifndef RPC_CLIENT_H_
#define RPC_CLIENT_H_

#include "rpc_types.h"
#include "rpc_proxy.h"
#include "rpc_util.h"
BEGIN_DECLS

struct rpc_client_s {
  rpc_int_t init_count;
	int last_thread;

  rpc_pool_t *pool;
	rpc_thread_worker_t *th_workers;

	pthread_cond_t cond;
	pthread_mutex_t mutex;
};

rpc_client_t* rpc_client_new();

void rpc_client_free(rpc_client_t *client);

END_DECLS

#endif /* RPC_CLIENT_H_ */
