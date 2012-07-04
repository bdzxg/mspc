/*
 * rpc_server.h
 *
 *  Created on: 2012-2-21
 *      Author: yanghu
 */

#ifndef RPC_SERVER_H_
#define RPC_SERVER_H_

#include "rpc_types.h"
#include "rpc_pool.h"
#include "rpc_connection.h"
#include "rpc_proxy.h"
#include "rpc_util.h"

BEGIN_DECLS

struct rpc_server_s {
  int init_count;
	int last_thread;
  int worker_count; //worker thread count 
  u_char is_main_dispatch_th; //main thread as dispatch thread?

  rpc_pool_t *pool;
  rpc_array_t *channels;

	rpc_thread_dispatch_t *th_dispatch;
	rpc_thread_worker_t *th_workers;
	rpc_hash_t *service_map;

	pthread_cond_t cond;
	pthread_mutex_t mutex;

};

struct rpc_service_s {
  char *key;
  char *service_name;
  char *method_name;

  rpc_function_ptr  cb;
};

struct rpc_channel_setting_s {
  int backlog;
  rpc_int_t max_conn; //最大连接数
  rpc_msec_t max_idle_time; //连接最大空闲时间
};

struct rpc_channel_s {
  int fd;
  rpc_channel_setting_t setting;
  rpc_endpoint_t  *ep;
  rpc_server_t *server;

  struct ev_loop *l;
  struct ev_io watcher;
};
 
rpc_server_t* rpc_server_new();

void rpc_server_free(rpc_server_t *svr);

void rpc_server_stop(rpc_server_t *server);


rpc_channel_t* rpc_server_regchannel(rpc_server_t *server, char *addr_text);

rpc_int_t rpc_server_regservice(rpc_server_t *server, char *service_name, char *method_name, rpc_function_ptr cb);

rpc_int_t rpc_server_start(rpc_server_t *server);


rpc_channel_t *rpc_channel_create(rpc_pool_t *pool,char *addr_text);

rpc_int_t rpc_channel_start(rpc_channel_t *channel, struct ev_loop *l);

void rpc_channel_stop(rpc_channel_t *channel);

//不是马上就发送出去，只是添加到缓冲区
rpc_int_t rpc_return_null(rpc_connection_t *c);

rpc_int_t rpc_return_error(rpc_connection_t *c,rpc_int_t err_code, char *msg, ...);

rpc_int_t rpc_return(rpc_connection_t *c, void *buf, size_t size);

END_DECLS
#endif /* RPC_SERVER_H_ */
