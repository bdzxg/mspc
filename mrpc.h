#ifndef __MRPC_H
#define __MRPC_H

#include <string.h>
#include "proxy.h"

typedef struct rpc_request_header_s {
} rpc_request_header_t;

typedef struct rpc_response_header_s {
} rpc_response_header_t;

typedef struct mrpc_message_s {
	int mask;
	int package_length;
	short header_length;
	short packet_options;
	union h {
		rpc_request_header_t req_head;
		rpc_response_header_t resp_head;
	};
	short is_resp;
	slice body;
}mrpc_message_t;

typedef struct mrpc_connection_s {
	int fd;
	struct buffer_s *send_buf;
	struct buffer_s *recv_buf;
	mrpc_message_t req;
}mmrpc_connection_t;

int mmrpc_upstreamer_init();
int mmrpc_upstreamer_start();
int mmrpc_upstreamer_send(upstream_t*, mmrpc_upstreamer_request_t*);
int mmrpc_send_response(mmrpc_connection_t*, mmrpc_message_t*);

#endif
