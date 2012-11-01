#ifndef _h_MRPC_H
#define __MRPC_H

#include <string.h>
#include "proxy.h"

#define MRPC_BUF_SIZE 4096
#define UP_CONN_COUNT 3
#define MRPC_CONN_DISCONNECTED 0
#define MRPC_CONN_CONNECTING 1
#define MRPC_CONN_CONNECTED 2
#define MRPC_CONN_FROZEN 3

typedef struct mrpc_upstreamer_s {
	int listen_fd;
	struct rb_root root;
	size_t count;
	list_head_t conn_list;
	size_t conn_count;
}mrpc_upstreamer_t;

typedef struct mrpc_us_item_s {
	char *uri;
	struct rb_node rbnode;
	list_head_t conn_list;
	int current_conn;
        size_t conn_count;
	list_head_t pending_list;
}mrpc_us_item_t;

typedef struct mrpc_message_s {
	uint32_t mask;
	uint32_t package_length;
	short header_length;
	short packet_options;
	union {
		mrpc_request_header req_head;
		mrpc_response_header resp_head;
	}h;
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
	time_t connecting;
	time_t connected;
	uint32_t seq;
	struct rb_root root;
	list_head_t list_us;
	list_head_t list_to;
	int conn_status;
}mrpc_connection_t;

int mrpc_init();
int mrpc_start();
int mrpc_us_send(rec_msg_t *);
void mrpc_us_send_err(rec_msg_t *);

#endif
