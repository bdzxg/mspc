#ifndef __MRPC_H
#define __MRPC_H

#include <string.h>
#include "proxy.h"

#define MRPC_BUF_SIZE 4096

typedef struct mrpc_message_s {
	uint32_t mask;
	uint32_t package_length;
	short header_length;
	short packet_options;
	union h {
		mrpc_request_header req_head;
		mrpc_response_header resp_head;
	};
	short is_resp;
	slice body;
}mrpc_message_t;

typedef struct mrpc_buf_s {
	char *buf;
	size_t len;  //buf len
	size_t size; //data size
	size_t offset;
}mrpc_buf_t;

typedef struct mrpc_connection_s {
	int fd;
	mrpc_buf_t *send_buf;
	mrpc_buf_t *recv_buf;
	mrpc_message_t req;
}mmrpc_connection_t;

int mmrpc_upstreamer_init();
int mmrpc_upstreamer_start();
int mmrpc_upstreamer_send(upstream_t*, mmrpc_upstreamer_request_t*);
int mmrpc_send_response(mmrpc_connection_t*, mmrpc_message_t*);

#endif
