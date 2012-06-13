
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_ENDPOINT_H_INCLUDED_
#define _RPC_ENDPOINT_H_INCLUDED_

#include "rpc_types.h"

struct rpc_endpoint_s {
  char  *addr_text;

  /* tcp uds */
  char  *protocol;
  char  *path;

  struct sockaddr *sockaddr;
  socklen_t socklen;

  rpc_pool_t  *pool;
};

rpc_endpoint_t *rpc_endpoint_new(rpc_pool_t *p, char *addr_text);

/* 
  uds:///var/uds.sock 
  tcp://192.168.110.231:8998
*/
rpc_int_t rpc_endpoint_parse(rpc_endpoint_t *ep);

#endif
