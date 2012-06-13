
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_NET_INCLUDED_
#define _RPC_NET_INCLUDED_ 

#include "rpc_types.h"

/* connect */
rpc_connection_t *rpc_connect(rpc_endpoint_t *ep);

/* test connect */
rpc_int_t rpc_connection_test(rpc_connection_t *c);

/* recv */
ssize_t rpc_recv(rpc_connection_t *c, u_char *buf, size_t size);

/* send */
ssize_t rpc_send(rpc_connection_t *c, u_char *buf, size_t size);
rpc_int_t rpc_packet_send(rpc_connection_t *c, rpc_packet_t *packet);

/* listening */
rpc_connection_t *rpc_listening(rpc_endpoint_t *ep);

/* accept */
void rpc_event_accept(rpc_event_t *ev);

/* recv packet */
void rpc_packet_process(rpc_event_t *ev);
#endif
