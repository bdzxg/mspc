#ifndef __MRPC_H
#define __MRPC_H

#include <string.h>
#include "msp_pb.h"
#include "proxy.h"

#define MRPC_BUF_SIZE 4096
#define UP_CONN_COUNT 3
#define MRPC_CONN_DISCONNECTED 0
#define MRPC_CONN_CONNECTING 1
#define MRPC_CONN_CONNECTED 2
#define MRPC_CONN_FROZEN 3
#define MRPC_CONN_DURATION 600
#define MRPC_TX_TO 180
#define MRPC_MAX_PENDING 10240
#define MRPC_CONNECT_TO 5


typedef struct mrpc_upstreamer_s {
	int listen_fd;
	list_head_t conn_list; //connecting list
	size_t conn_count;
}mrpc_upstreamer_t;

typedef struct mrpc_us_item_s {
	char *uri;
	struct rb_node rbnode;
	list_head_t conn_list; //connection list 
	list_head_t frozen_list; //frozen connection list
	int current_conn;
        size_t conn_count;
	list_head_t pending_list; // pending request list 
	size_t pend_count;
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
	struct 	pbc_slice body;
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
	mrpc_us_item_t * us;
	ev_file_item_t *event;
}mrpc_connection_t;

typedef struct mrpc_stash_req_s {
	struct rb_node rbnode;
	uint32_t seq;
	int32_t user_id;
	char *epid;
	uint32_t mcp_seq;
} mrpc_stash_req_t;

int mrpc_init();
int mrpc_start();
int mrpc_us_send(rec_msg_t *);
void mrpc_us_send_err(rec_msg_t *);
void mrpc_ev_after();
void mrpc_svr_accept(ev_t *, ev_file_item_t*);

mrpc_connection_t* mrpc_conn_new(mrpc_us_item_t *us);
void mrpc_conn_close(mrpc_connection_t *c);
void mrpc_conn_free(mrpc_connection_t *c);
int mrpc_send(mrpc_connection_t *c);
int mrpc_recv(mrpc_connection_t *c);
//1 OK, 0 not finish, -1 error
int 
mrpc_parse(mrpc_buf_t *b, mrpc_message_t *msg, mcp_appbean_proto *proto);
mrpc_buf_t* mrpc_buf_new(size_t size);
int mrpc_buf_enlarge(mrpc_buf_t *buf);
void mrpc_buf_reset(mrpc_buf_t *buf);




#endif
