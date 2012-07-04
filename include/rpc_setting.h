/*
 * rpc_client.h
 *
 *  Created on: 2012-2-21
 *      Author: yanghu
 */

#ifndef RPC_SETTING_H_
#define RPC_SETTING_H_ 

#include "rpc_types.h"

BEGIN_DECLS

#ifndef IOV_MAX
#define IOV_MAX 1024
#endif

#ifdef MAXHOSTNAMELEN
#define rpc_max_host_len MAXHOSTNAMELEN
#else
#define rpc_max_host_len 256
#endif

/* server side worker count */
/* worker count 5 for 200k */
#ifndef rpc_worker_count
#define rpc_worker_count 5
#endif

/* client side worker count */
#ifndef rpc_req_count
#define rpc_req_count 4
#endif

/* client side proxy connection count */
#ifndef rpc_conn_count
#define rpc_conn_count 3
#endif

/* client side req server name */
#ifndef rpc_server_name
#define rpc_server_name NULL
#endif

/* client side req service name */
#ifndef rpc_service_name
#define rpc_service_name "RPC_C_SERVICE"
#endif

#ifndef rpc_loop_per_req
#define rpc_loop_per_req 64
#endif 

#ifndef rpc_loop_per_rsp
#define rpc_loop_per_rsp -1
#endif 

#ifndef rpc_conn_pool_size
#define rpc_conn_pool_size 1024
#endif

#ifndef rpc_conn_buf_nalloc
#define rpc_conn_buf_nalloc 4
#endif

/* 2K */
#ifndef rpc_conn_buf_size
#define rpc_conn_buf_size 2048
#endif

#ifndef rpc_conn_iov_size
#define rpc_conn_iov_size 512
#endif

#ifndef rpc_conn_msg_size
#define rpc_conn_msg_size 8
#endif

#ifndef rpc_conn_max_iov_size
#define rpc_conn_max_iov_size 8192
#endif

#ifndef rpc_conn_idle_time
#define rpc_conn_idle_time 3600
#endif

#ifndef rpc_conn_pending_size
#define rpc_conn_pending_size (1* 12 * 1024)
#endif 

#ifndef rpc_request_default_timeout
#define rpc_request_default_timeout (360 * 1000)
#endif 

#ifndef rpc_connect_default_timeout
#define rpc_connect_default_timeout 500
#endif

#ifndef rpc_backlog
#define rpc_backlog 32
#endif

END_DECLS
#endif

