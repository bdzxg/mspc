#include "proxy.h"
#include "mrpc.h"

static mrpc_buf_t* mrpc_buf_new(size_t size)
{
	mrpc_buf_t *b = calloc(1, sizeof(*b));
	if(!b) {
		E("no mem for mrpc_buf_t");
		return NULL;
	}

	b->buf = malloc(size);
	if(!b->buf) {
		E("no mem for buf");
		free(b);
		return NULL;
	}
	b->len = size;
	return b;
}

static int mrpc_buf_enlarge(mrpc_buf_t *buf) 
{
	void *n = realloc(buf->buf, buf->len * 2);
	if(n) {
		buf->buf = n;
		buf->size = buf->size * 2;
		return 0;
	}
	return -1;
}

static void mrpc_buf_reset(mrpc_buf_t *buf)
{
	buf->offset = 0;
	buf->size = 0;
	//TODO: shrink the buf len
}


staitc int rpc_send_responnse(rpc_upstreamer_connection_t *c, rpc_response_t *resp)
{
	buffer_t *b = get_buf_for_write(c);
	if(!b) {
		E("can not get the buffer");
		return -1;
	}

	//write the reponse data to the buffer, then send it 
}

static inline mrpc_resp_from_req(mrpc_message_t *req, mrpc_message_t *resp)
{
	resp->mask = htonl(0xBADBEE01);
	resp->h.resp_head.sequence = req->h.req_head.sequence;
	resp->h.resp_head.to_id = req->h.req_head.to_id;
	resp->h.resp_head.from_id = req->h.req_head.from_id;
	resp->h.resp_head.body_length = 0;
}

static int rpc_process_client_req(rpc_upstreamer_connection_t *c)
{
	size_t readn = 0;
	int index = 0, buf_len = 0;
	char *c1,*c2,*c3,*c4;
	mrpc_message_t msg;
	mrpc_buf_t *b = c->recv_buf;

	//parse the rpc packet
	if((buf_len = buffer_length(c->recv_buf)) < 12) {
		//not a full package
		return 0;
	}

	//mask:
	msg.mask = ntohl(*(uint32_t*)(b->buf + b->offset));
	b->offset += 4;
	//TODO: check the mask

	//package length
	msg.package_length = ntohl(*(uint32_t*)(b->buf + b->offset));
	b->offset += 4;

	//not a full package
	if(b->size < msg.package_length) {
		return 0;
	}

	//header length
	msg.header_length = ntohs(*(short)(b->buf + b->offset));
	b->offset += 2;
	
	//skip the packet options
	b->offset += 2;

	//header
	char *header = malloc(msg.header_length);
	if(!header) {
		E("malloc header error");
		return;
	}
	memcpy(header, b->buf + b->offset, msg.header_length);
	b->offset += msg.header_length;

	mrpc_request_header reqh;
	pbc_slice str = {header, msg.header_length};
	int r = pbc_pattern_unpack(rpc_pat_mrpc_request_header, &str, &reqh);
	if(r < 0) {
		//TODO: ERROR REPONSE
	}

	//body
	size_t body_len = msg.package_length - msg.header_length;
	char *body = malloc(body_len);
	if(!body) {
		E("malloc body error");
		return;
	}
	memcpy(body, b->buf + b-> offset, body_len);


	//pb deserialize
	mcp_appbean_proto input;
	rpc_pb_string str = {body, body_len};
	int r = rpc_pb_pattern_unpack(rpc_pat_mcp_appbean_proto, &str, &input);
	if ( r < 0) {
		//TODO: handle error
		rpc_return_error(c, RPC_CODE_INVALID_REQUEST_ARGS, "input decode failed!");
		return;
	}

	rpc_message_t resp;
	mrpc_buf *sbuf = c->send_buf;
	mrpc_resp_from_req(&msg, &resp);
	char temp[32];
	pbc_slice obuf = {temp, 32};
	r = pbc_pattern_pack(rpc_pat_mrpc_response_header, &resp.h.resp_head, &obuf);
	if(r < 0) {
		//TODO
	}
	uint32_t resp_size = 12 + 32 - r;
	while(c->send_buf->size + resp_size > c->send_buf->len) {
		mrpc_buf_enlarge(c->send_buf);
	}

	//write the response to the send_buf
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htonl(0XBADBEE01);
	sbuf->size += 4;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htonl(resp_size);
	sbuf->size += 4;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htons(32- r);
	sbuf->size += 2;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htons(0);
	sbuf->size += 2;
	memcpy(sbuf->buf + sbuf->size, temp, 32 - r);

	int n = send(c->fd, sbuf->buf + sbuf->offset, sbuf->size - sbuf->offset, 0);
	if(n > 0 ) {
		sbuf->offset += n;
	}
	else {
		//TODO
	}
	
	process_bn(&msg, a);
	W("BN epid %s!", msg.epid);
}

void mrpc_recv(ev_t *ev,ev_file_item_t *fi)
{
	UNUSED(ev);
	int n;
	mrpc_connection_t *c = fi->data;
	mrpc_buf_t *b = c->recv_buf;

	D("rpc server recv"); 
	if(!c)	{
		//TODO:
		W("fd has no rpc_connection ,ev->data is NULL,close the fd");
		close(fi->fd); return;
	}

	size_t buf_avail;
	while(1) {
		buf_avail = b->len - b->size;
		n = recv(fi->fd, b->buf + b->size, buf_avail, 0);
		D("recv %d bytes",n);

		if(n < 0) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			else {
				W("read error,errno is %d",errno);
				goto failed;
			}
		}

		if(n == 0) {
			W("socket fd #%d closed by peer",fi->fd);
			goto failed;
		}

		b->size    += n;

		if(n < buf_avail) {	
			D("break from receive loop, n > BUFFER_SIZE");	
			break;
		}
		
		//enlarge the recv_buf
		int t = mrpc_buf_enlarge(b);
		if(t < 0) {
			E("recv_buf enlarge error");
			goto failed;
		}
	}

	D("buffer-len %zu", agent->buf_len);
	if(mrpc_process_client_req(c) < 0) {
		W("rpc: process client request failed");
		goto failed;
	}
	return;

failed:
	W("failed, prepare close!");
	//TODO: clean the connection, and remove the event
	close(fi->fd);
	free(fi);
	return;
}

static void mrpc_upstreamer_accept (ev_t *ev, ev_file_item_t *ffi)
{
	UNUSED(ev);

	int f,err;
	pxy_agent_t *agent;
	ev_file_item_t *fi;

	socklen_t sin_size = sizeof(master->addr);
	//f = accept(ffi->fd,&(master->addr),&sin_size);
	f = accept(ffi->fd,NULL,NULL);
	D("rpc server: fd#%d accepted",f);

	if(f>0){

		err = setnonblocking(f);
		if(err < 0){
			E("set nonblocking error"); return;
		}

		rpc_upstreamer_connection_t *c = calloc(1, sizeof(*c));
		if(!c) {
			E("malloc upstream connection error");
			return;
		}
		c->send_buf = mrpc_buf_new(MRPC_BUF_SIZE);
		if(!c->send_buf) {
			//TODO
			return;
		}
		c->recv_buf = mrpc_buf_new(MRPC_BUF_SIZE);
		if(!c->recv_buf) {
			//TODO
			return;
		}
		c->fd = f;

		fi = ev_file_item_new(f,
				      c,
				      mrpc_recv,
				      mrpc_send,
				      EV_WRITABLE | EV_READABLE | EPOLLET);
		if(!fi){
			E("create file item error");
		}
		ev_add_file_item(worker->ev,fi);

		//map_insert(&worker->root,agent);
		/*TODO: check the result*/
	}	
	else {
		perror("accept");
	}
}


int mrpc_upstreamer_start()
{
	ev_file_item_t* fi ;
	int fd = upstreamer.fd;

	if(listen(fd, 1024) < 0){
		E("rpc listen error");
		return -1;
	}

	fi = ev_file_item_new(fd, worker, mrpc_upstreamer_accept, NULL, EV_READABLE);
	if(!fi){
		E("create ev for listen fd error");
		goto start_failed;
	}
	ev_add_file_item(worker->ev,fi);


start_failed:
	return -1;
}


int mrpc_upstreamer_init()
{
	struct sockaddr_in addr1;

	upstreamer.fd = -1;
	
	upstreamer.fd = socket(AF_INET, SOCK_STREAM, 0);
	if(upstreamer.fd < 0)  {
		E("create rpc listen fd error");
		return -1;
	}
	int reuse = 1;
	setsockopt(upstreamer.fd , SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	
	if(setnonblocking(upstreamer.fd) < 0){
		E("set nonblocling error");
		return -1;
	}

	addr1.sin_family = AF_INET;
	addr1.sin_port = htons(config->backend_port);
	addr1.sin_addr.s_addr = 0;
	
	if(bind(upstreamer.fd, (struct sockaddr*)&addr1, sizeof(addr1)) < 0){
		E("bind error");
		return -1;
	}
	return 0;
}



