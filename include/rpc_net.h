
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_NET_INCLUDED_
#define _RPC_NET_INCLUDED_ 

#include "rpc_types.h"

/* connect */
rpc_int_t rpc_connect(rpc_endpoint_t *ep, rpc_msec_t timer);

/* recv */
ssize_t rpc_recv(rpc_connection_t *c, u_char *buf, size_t size);

#endif
