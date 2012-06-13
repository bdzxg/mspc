
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_CLIENT_H_INCLUDED_
#define _RPC_CLIENT_H_INCLUDED_

#include "rpc_types.h"

#define RPC_DEFAULT_TIME_OUT  (rpc_msec_t)1000

struct rpc_client_s {
  rpc_msec_t time_out;
  rpc_endpoint_t *ep;
  rpc_connection_t *connection;
  rpc_packet_t  *packet;
  rpc_pool_t *pool;
};

void rpc_stack_init(char *service_name);

void rpc_stack_init_full(char *service_name, char *computer_name);

#define rpc_client_new(addr) rpc_client_new_full(addr,RPC_DEFAULT_TIME_OUT)

rpc_client_t *rpc_client_new_full(char *addr_text, rpc_msec_t time_out);

rpc_int_t rpc_client_send_request(rpc_client_t *client,
    char *service_name, char *method_name,
    void *input ,size_t input_size,
    void **output,size_t *output_size);

void rpc_client_close(rpc_client_t *client);

void rpc_client_free(rpc_client_t *client);

#endif 
