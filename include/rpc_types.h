
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_TYPES_H_INCLUDED_
#define _RPC_TYPES_H_INCLUDED_

#ifdef  __cplusplus
# define BEGIN_DECLS  extern "C" {
# define END_DECLS    }
#else
# define BEGIN_DECLS
# define END_DECLS
#endif

#define rpc_abs(value)  (((value) >= 0) ? (value) : - (value))
#define rpc_max(val1, val2) ((val1 < val2) ? (val2) : (val1))
#define rpc_min(val1, val2) ((val1 > val2) ? (val2) : (val1))

#define rpc_ptoi(p) ((int)  (long) (p))
#define rpc_itop(i) ((void *) (long) (i))

#ifndef __need_IOV_MAX
#define __need_IOV_MAX
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include <assert.h>
#include <ev.h>

#include "rpc_log.h"
#include "pbc.h"

BEGIN_DECLS

/* RPC types */
typedef intptr_t rpc_int_t;
typedef uintptr_t rpc_uint_t;
typedef rpc_uint_t rpc_msec_t;
typedef rpc_int_t rpc_msec_int_t;

typedef struct rpc_queue_s rpc_queue_t;
typedef struct rpc_queue_item_s rpc_queue_item_t;
typedef struct rpc_array_s rpc_array_t;
typedef struct rpc_hash_s rpc_hash_t;
typedef struct rpc_sessionpool_s rpc_sessionpool_t;

typedef struct rpc_buf_s  rpc_buf_t;
typedef struct rpc_pool_s rpc_pool_t;
typedef struct rpc_endpoint_s rpc_endpoint_t;
typedef struct rpc_connection_s rpc_connection_t;

typedef struct rpc_client_s rpc_client_t;
typedef struct rpc_proxy_s rpc_proxy_t;

typedef struct rpc_channel_s rpc_channel_t;
typedef struct rpc_server_s rpc_server_t;
typedef struct rpc_service_s rpc_service_t;

typedef struct rpc_thread_dispatch_s rpc_thread_dispatch_t;
typedef struct rpc_thread_worker_s rpc_thread_worker_t;

typedef struct rpc_channel_setting_s rpc_channel_setting_t;
typedef struct rpc_proxy_setting_s rpc_proxy_setting_t;

typedef struct rpc_request_s rpc_request_t;

/* pb */
typedef struct pbc_slice rpc_pb_string;
typedef pbc_array rpc_pb_array;
typedef struct pbc_env rpc_pb_env;
typedef struct pbc_pattern rpc_pb_pattern;

#define rpc_pb_env_new pbc_new 
#define rpc_pb_env_regdesc pbc_register

#define rpc_pb_pattern_new pbc_pattern_new
#define rpc_pb_pattern_pack pbc_pattern_pack
#define rpc_pb_pattern_unpack pbc_pattern_unpack

#define rpc_pb_array_size pbc_array_size
#define rpc_pb_array_slice pbc_array_slice
#define rpc_pb_array_real pbc_array_real
#define rpc_pb_array_integer pbc_array_integer

#define rpc_pb_array_push_integer pbc_array_push_integer
#define rpc_pb_array_push_real pbc_array_push_real 
#define rpc_pb_array_push_slice pbc_array_push_slice

#define rpc_pb_pattern_close_array pbc_pattern_close_arrays 

typedef void (*rpc_function_ptr)(rpc_connection_t *c, void *input, size_t input_len);
typedef void (*rpc_callback_ptr)(rpc_connection_t *c,rpc_int_t code, void *output, size_t output_len, void *input, size_t input_len, void *data);
/* RPC mem alloc*/
#define RPC_TIMER_INFINITE (rpc_msec_t) -1

#ifndef RPC_ALIGNMENT
#define RPC_ALIGNMENT sizeof(unsigned long)
#endif

#define rpc_alloc malloc
#define rpc_calloc  calloc 
#define rpc_free  free
#define rpc_realloc realloc

#define rpc_strcpy(dst, src)  strcpy(dst, src)
#define rpc_strlen(s) strlen(s)
#define rpc_memzero(buf, n) (void) memset(buf, 0 ,n)
#define rpc_memset(buf, c, n) (void) memset(buf, c, n)
#define rpc_memcpy(dst, src, n) (void) memcpy(dst, src, n)
#define rpc_memmove(dst, src, n) (void) memmove(dst, src, n)

#define rpc_set_nonblock(fd) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)
#define rpc_set_cloexec(fd) fcntl(fd, F_SETFD, FD_CLOEXEC, 1)

/* RPC return code */
#define RPC_OK 0
#define RPC_ERROR -1
#define RPC_AGAIN -2
#define RPC_BUSY -3
#define RPC_DONE -4
#define RPC_DECLINED -5
#define RPC_ABORT -6
#define RPC_NONE -999

/* RPC error code */
#define RPC_CODE_OK 200
#define RPC_CODE_SEND_FAILED -1
#define RPC_CODE_SEND_PENDING -2
#define RPC_CODE_TRANSACTION_TIMEOUT -3
#define RPC_CODE_TRANSACTION_BUSY -4
#define RPC_CODE_CONNECTION_TIMEOUT -5
#define RPC_CODE_CONNECTION_BROKEN -6
#define RPC_CODE_CONNECTION_PENDING -7
#define RPC_CODE_SERVICE_NOT_FOUND 404
#define RPC_CODE_METHOD_NOT_FOUND 405
#define RPC_CODE_CONNECTION_FAILED 481
#define RPC_CODE_SERVER_ERROR 500
#define RPC_CODE_SERVER_BUSY 503
#define RPC_CODE_SERVER_TIMEOUT 504
#define RPC_CODE_INVALID_REQUEST_ARGS 512
#define RPC_CODE_INVALID_RESPONSE_ARGS 513
#define RPC_CODE_SERVER_TRANSFER_FAILED 600
#define RPC_CODE_UNKOWN 699

END_DECLS
#endif
