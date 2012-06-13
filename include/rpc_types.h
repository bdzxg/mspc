
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_TYPES_H_INCLUDED_
#define _RPC_TYPES_H_INCLUDED_

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
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

/* RPC types */
typedef intptr_t rpc_int_t;
typedef uintptr_t rpc_uint_t;
typedef rpc_uint_t rpc_msec_t;
typedef rpc_int_t rpc_msec_int_t;

typedef struct rpc_buf_s  rpc_buf_t;
typedef struct rpc_pool_s rpc_pool_t;
typedef struct rpc_endpoint_s rpc_endpoint_t;
typedef struct rpc_connection_s rpc_connection_t;
typedef struct rpc_packet_s rpc_packet_t;
typedef struct rpc_event_s rpc_event_t;
typedef struct rpc_stack_s rpc_stack_t; 
typedef struct rpc_client_s rpc_client_t;
typedef struct rpc_server_s rpc_server_t;
typedef struct rpc_service_s rpc_service_t;

typedef void (*rpc_callback_ptr)(rpc_connection_t *c, void *input, size_t input_size);

struct rpc_stack_s {
  char *service_name;
  char *computer_name;

  rpc_event_t *read_events ;
  rpc_event_t *write_events ;

  rpc_connection_t *connections ;
  rpc_uint_t connection_n ;

  rpc_connection_t *free_connections ;
  rpc_uint_t free_connection_n ;
};

extern rpc_stack_t *rpc_stack;
extern rpc_server_t *rpc_current_server;

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

#define rpc_align(d, a) (((d) + (a - 1)) & ~(a - 1))
#define rpc_align_ptr(p, a) \
  (u_char *) (((uintptr_t) (p) + ((uintptr_t) a -1)) & ~((uintptr_t) a - 1))

/* RPC return code */
#define RPC_OK 0
#define RPC_ERROR -1
#define RPC_AGAIN -2
#define RPC_BUSY -3
#define RPC_DONE -4
#define RPC_DECLINED -5
#define RPC_ABORT -6

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

#define rpc_abs(value)  (((value) >= 0) ? (value) : - (value))
#define rpc_max(val1, val2) ((val1 < val2) ? (val2) : (val1))
#define rpc_min(val1, val2) ((val1 > val2) ? (val2) : (val1))

#endif
