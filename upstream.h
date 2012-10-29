
#ifndef __UPSTEAM_H
#define __UPSTEAM_H

#include <string.h>
#include "proxy.h"
#include "include/rpc_client.h"

typedef struct upstream_s {
	char *uri;
	rpc_proxy_t *proxy;
	struct rb_node rbnode;
}upstream_t;

typedef struct upstream_map_s {
	struct rb_root root;
	size_t count;
} upstream_map_t;

int us_add_upstream(upstream_map_t*, upstream_t*);
upstream_t* us_get_upstream(upstream_map_t *, char*);
void release_upstream(upstream_t* us);







#endif


#ifndef __UPSTEAM_H
#define __UPSTEAM_H

#include <string.h>
#include "proxy.h"
#include "include/rpc_client.h"

static uint rpc_upstreamer_sequence;
rpc_upstreamer_t upstreamer = { 0 }; 

typedef struct upstream_s {
	char *uri;
	rpc_proxy_t *proxy;
	struct rb_node rbnode;
	struct upstreamer_send_queue_s *q;
	size_t q_count;
	struct rpc_upstreamer_connection_s *conn;
}upstream_t;

typedef struct upstream_map_s {
	struct rb_root root;
	size_t count;
} upstream_map_t;

typedef struct upstreamer_send_queue_s {
	list_head_t head;
	void *data;
} upstreamer_send_queue_t;

typedef struct rpc_request_header_s {
} rpc_request_header_t;

typedef struct rpc_response_header_s {
} rpc_response_header_t;

typedef struct rpc_message_s {
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
}rpc_message_t;

typedef struct mrpc_connection_s {
	
}mmrpc_connection_t;

typedef struct mrpc_upstreamer_s {
	int fd;
} mrpc_upstreamer_t;

typedef struct mrpc_tx_map_s {
	size_t count;
	struct rb_root root;
} mrpc_tx_map_t;

typedef struct mrpc_tx_s {
	int seq;
	struct mrpc_upstreamer_request_s *req;
	struct mrpc_upstreamer_response_s *resp;
	time_t tm;
} mrpc_tx_t;

typedef struct mrpc_upstreamer_connection_s {
	int fd;
	struct buffer_s *send_buf;
	struct buffer_s *recv_buf;
	mrpc_message_t req;
} mrpc_upstreamer_connection_t;

typedef struct mrpc_upstreamer_request_s {
	char *data;
	size_t len;
	int retry_count;
} mrpc_upstreamer_request_t;

typedef struct mrpc_upstreamer_response_s {
	int status_code;
	char *data;
	size_t len;
} mrpc_upstreamer_response_t;

int us_add_upstream(upstream_map_t*, upstream_t*);
upstream_t* us_get_upstream(upstream_map_t *, char*);
void release_upstream(upstream_t* us);

int mrpc_upstreamer_init();
int mrpc_upstreamer_start();
int mrpc_upstreamer_send(upstream_t*, mrpc_upstreamer_request_t*);
#endif
