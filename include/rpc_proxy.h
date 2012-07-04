/*
 * rpc_proxy.h
 *
 *  Created on: 2012-2-29
 *      Author: yanghu
 */

#ifndef RPC_PROXY_H_
#define RPC_PROXY_H_

#include "rpc_types.h"

BEGIN_DECLS

struct rpc_proxy_setting_s {
  size_t     request_pending_size;
  rpc_msec_t connect_timeout;      //connect time out  ms
  rpc_msec_t request_timeout;      //request time out -1 : none ms
  rpc_msec_t connection_idle_time; // connection max idle time -1 :none s
};

//TODO
//stat
struct rpc_stat_s {
  pthread_mutex_t mutex;
  uint64_t      curr_bytes;
  unsigned int  curr_conns;
  unsigned int  total_conns;
  uint64_t      rejected_conns;                                                                                                                  
  unsigned int  reserved_fds;
  unsigned int  conn_structs;

  uint64_t req_total_count;
  uint64_t req_success_count;
  uint64_t req_fail_count;

  time_t started;
}rpc_stat_t;

struct rpc_proxy_s{
	int last_conn;

  int req_total_count;
  int req_success_count;
  int req_fail_count;

  rpc_uint_t conn_count;
  rpc_proxy_setting_t setting;

  rpc_client_t *client;
  rpc_pool_t *pool;
  rpc_endpoint_t *addr;

	rpc_sessionpool_t *req_sended;  

  rpc_array_t *connections;
  struct ev_loop *loop;

	pthread_cond_t cond;
	pthread_mutex_t mutex;
};

#define rpc_client_connect(c,addr_text) rpc_client_connect_data(c, addr_text, -1)

rpc_proxy_t* rpc_client_connect_data(rpc_client_t *c, void *addr_data, int len);

#define rpc_proxy_new(addr_text) rpc_proxy_new_data(addr_text, -1)

rpc_proxy_t* rpc_proxy_new_data(void *addr_data, int len);

void rpc_proxy_free(rpc_proxy_t *p);

void rpc_proxy_close(rpc_proxy_t *p);

rpc_int_t rpc_call(rpc_proxy_t *p, char *service_name,
		char *method_name, void *input, size_t input_len, void **output,
		size_t *output_len);

void rpc_call_async(rpc_proxy_t *p, char *service_name,
		char *method_name, void *input, size_t input_len,
		rpc_callback_ptr callback, void *data);

END_DECLS

#endif /* RPC_PROXY_H_ */
