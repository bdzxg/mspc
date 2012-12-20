#include "proxy.h"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;
extern pxy_config_t* config;
static char* PROTOCOL = "MCP/3.0";


//mcp_appbean_proto.protocol = "mcp3.0"
char __resp[9] = {0x0a, 0x07, 0x4d, 0x43, 0x50, 0x2f, 0x33, 0x2e, 0x30};

static inline 
void mrpc_resp_from_req(mrpc_message_t *req, mrpc_message_t *resp)
{
	resp->mask = htonl(0xBADBEE01);
	resp->h.resp_head.sequence = req->h.req_head.sequence;
	D("the resp seq is %d-%d",req->h.req_head.sequence,
	  resp->h.resp_head.sequence);
	resp->h.resp_head.to_id = req->h.req_head.to_id;
	resp->h.resp_head.from_id = req->h.req_head.from_id;
	resp->h.resp_head.opt = 0;
	resp->h.resp_head.body_length = 0;
}

//TODO can we remove the malloc(proto) ?
static int mrpc_process_client_req(mrpc_connection_t *c)
{
	mrpc_message_t msg;
	mrpc_buf_t *b = c->recv_buf;
	mcp_appbean_proto proto;

	int r = mrpc_parse(b, &msg, &proto);
	
	if(r < 0) {
		goto failed;
	}
	if(r == 0) {
		return 0;
	}

	//send the response
	mrpc_message_t resp;
	mrpc_buf_t *sbuf = c->send_buf;
	mrpc_resp_from_req(&msg, &resp);
	resp.h.resp_head.response_code = 200;
	resp.h.resp_head.body_length = 10;

	char temp[32];
	struct pbc_slice obuf = {temp, 32};
	r = pbc_pattern_pack(rpc_pat_mrpc_response_header, &resp.h.resp_head, &obuf);
	if(r < 0) {
		E("rpc pack response header error");
		goto failed;
	}
	uint32_t resp_size = 12 + 32 - r + sizeof(__resp);
	while(c->send_buf->size + resp_size > c->send_buf->len) {
		mrpc_buf_enlarge(c->send_buf);
	}

	//write the response to the send_buf
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htonl(0X0BADBEE1);
	sbuf->size += 4;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htonl(resp_size);
	sbuf->size += 4;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htons(32- r);
	sbuf->size += 2;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htons(0);
	sbuf->size += 2;
	memcpy(sbuf->buf + sbuf->size, temp, 32 - r);
	sbuf->size += 32 - r;
	memcpy(sbuf->buf + sbuf->size, __resp, 9);
	sbuf->size += 9;
	
	int n = mrpc_send(c);
	if(n < 0) {
		//TODO: goto failed?
		goto failed;
	}

	agent_mrpc_handler(&proto);
	
	//reset the recv buf, send_buf will be reset by _send function
	if(b->offset == b->size) {
		mrpc_buf_reset(b);
	}

	return 1;

failed:
	mrpc_conn_close(c);
	mrpc_conn_free(c);
	return -1;
}

static int mrpc_svr_recv(ev_t *ev,ev_file_item_t *fi)
{
	UNUSED(ev);
	int n;
	mrpc_connection_t *c = fi->data;
	D("rpc server recv"); 
	n = mrpc_recv(c);
	if(n == 0) {
		goto failed;
	}
	if( n == -1) {
		E("recv return -1");
		return 0;
	}

	int r;
	for(;;) {
		r = mrpc_process_client_req(c);
		if(r < 0) {
			W("Process client req error");
			goto failed;
		}
		else if (r == 0) {
			break;
		}
		else {}
	}
	return 0;

failed:
	W("failed, prepare close!");
	mrpc_conn_close(c);
	mrpc_conn_free(c);
	return -1;
}

static int mrpc_svr_send(ev_t *ev, ev_file_item_t *ffi)
{
	UNUSED(ev);
	D("mrpc_svr_send begins");
	mrpc_connection_t *c = ffi->data;
	if(mrpc_send(c) < 0) 
		goto failed;
	return 0;

failed:
	mrpc_conn_close(c);
	mrpc_conn_free(c);
	return -1;
}


int mrpc_svr_accept(ev_t *ev, ev_file_item_t *ffi)
{
	UNUSED(ev);

	int f,err;
	ev_file_item_t *fi;
	mrpc_connection_t *c = NULL;

	//socklen_t sin_size = sizeof(master->addr);
	//f = accept(ffi->fd,&(master->addr),&sin_size);
	f = accept(ffi->fd,NULL,NULL);
	D("rpc server: fd#%d accepted",f);

	if(f>0){
		err = setnonblocking(f);
		if(err < 0){
			E("set nonblocking error"); return 0;
		}
		c = mrpc_conn_new(NULL);
		c->fd = f;
		fi = ev_file_item_new(f,
				      c,
				      mrpc_svr_recv,
				      mrpc_svr_send,
				      EV_WRITABLE | EV_READABLE | EPOLLET);
		if(!fi){
			E("create file item error");
			goto failed;
		}
		if(ev_add_file_item(worker->ev,fi) < 0) {
			E("add file item failed");
			goto failed;
		}
		c->event = fi;
	}	
	else {
		E("accept error %s", strerror(errno));
	}
	return 0;

failed:
	if(c) {
		mrpc_conn_close(c);
		mrpc_conn_free(c);
	}
	return 0;
}

