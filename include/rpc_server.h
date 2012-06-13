
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_SERVER_H_INCLUDED_
#define _RPC_SERVER_H_INCLUDED_

#include "rpc_hash.h"

struct rpc_server_s {

  rpc_endpoint_t *ep;
  rpc_connection_t *listening;

  rpc_msec_t  time_out;
  rpc_hash_t  *service_map;
  rpc_pool_t *pool;
  u_char closed;
};

struct rpc_service_s {
  char *service_name;
  size_t service_name_len;

  char *method_name;
  size_t method_name_len;

  char *key;

  rpc_callback_ptr  cb;
};
  
#define rpc_server_new(s) rpc_server_new_full(s, 500)

rpc_server_t *rpc_server_new_full(char *addr_text, rpc_msec_t time_out);

rpc_int_t rpc_server_start(rpc_server_t *server);

void rpc_server_stop(rpc_server_t *server);

void rpc_server_free(rpc_server_t *server);

rpc_int_t rpc_server_register(rpc_server_t *server,char *service_name, char *method_name, rpc_callback_ptr cb);

rpc_int_t rpc_return(rpc_connection_t *c, void *buf, size_t size);

rpc_int_t rpc_return_null(rpc_connection_t *c);

rpc_int_t rpc_return_error(rpc_connection_t *c,rpc_int_t err_code, char *msg, ...);

#endif
