
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_CONNECTION_H_INCLUDED_
#define _RPC_CONNECTION_H_INCLUDED_

#include "rpc_types.h"

struct rpc_connection_s {
  void *data;
  rpc_event_t *read;
  rpc_event_t *write;

  int fd;
  rpc_pool_t *pool;

  struct sockaddr *sockaddr;
  socklen_t socklen;
  char *addr_text;

  struct sockaddr *local_sockaddr;
  rpc_buf_t *buffer;

  off_t sent;

};

rpc_uint_t rpc_connection_init();

rpc_connection_t *rpc_connection_get(int fd);

void rpc_connection_free(rpc_connection_t *c);

void rpc_connection_close(rpc_connection_t *c);

#endif
