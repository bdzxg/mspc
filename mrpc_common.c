#include "proxy.h"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;
static char* PROTOCOL = "MCP/3.0";


mrpc_buf_t* mrpc_buf_new(size_t size)
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

//TODO check the result
int mrpc_buf_enlarge(mrpc_buf_t *buf) 
{
	void *n = realloc(buf->buf, buf->len * 2);
	if(n) {
		buf->buf = n;
		buf->len = buf->len * 2;
		return 0;
	}
	return -1;
}

void mrpc_buf_reset(mrpc_buf_t *buf)
{
	buf->offset = 0;
	buf->size = 0;

	//shrink the buf
	if(buf->len > MRPC_BUF_SIZE) {
		void *t = realloc(buf->buf, MRPC_BUF_SIZE);
		if(t) {
			buf->buf = t;
			buf->len = MRPC_BUF_SIZE;
		}
		else {
			E("realloc for buf reset error");
		}
	}
}

void mrpc_buf_free(mrpc_buf_t *buf) 
{
	free(buf->buf);
	free(buf);
}


mrpc_connection_t* mrpc_conn_new(mrpc_us_item_t* us)
{
	mrpc_connection_t *c = NULL;

	c = calloc(1, sizeof(*c));
	if(!c) {
		E("malloc upstream connection error");
		return NULL;
	}
	c->send_buf = mrpc_buf_new(MRPC_BUF_SIZE);
	if(!c->send_buf) {
		E("cannot malloc send_buf!");
		free(c);
	}
	c->recv_buf = mrpc_buf_new(MRPC_BUF_SIZE);
	if(!c->recv_buf) {
		E("cannot malloc recv_buf");
		free(c->send_buf);
		free(c);
	}
	c->fd = -1;
	c->us = us;
	c->conn_status = MRPC_CONN_DISCONNECTED;
	
	INIT_LIST_HEAD(&c->list_us);
	INIT_LIST_HEAD(&c->list_to);
	c->root = RB_ROOT;
	return c;
}

void mrpc_conn_close(mrpc_connection_t *c)
{
	if(ev_del_file_item(worker->ev, c->event) < 0){
		E("remove event file item error %p, reason %s, %d %d",
		  c,strerror(errno), worker->ev->fd, c->event->fd);
	}	
	ev_file_free(c->event);
	close(c->fd);
	c->conn_status = MRPC_CONN_DISCONNECTED;
}

void mrpc_conn_free(mrpc_connection_t *c)
{
	mrpc_buf_free(c->send_buf);
	mrpc_buf_free(c->recv_buf);
	list_del(&c->list_us);
	list_del(&c->list_to);
	free(c);
}

int mrpc_send2(mrpc_buf_t *b, int fd)
{
	int r = 0,n;
	while(b->offset < b->size) {
		n = send(fd, b->buf + b->offset, b->size - b->offset, MSG_NOSIGNAL);
		D("sent %d bytes, b->offset %lu, b->size %lu", n, b->offset, b->size);
		if(n > 0) {
			b->offset += n;
			r += n;
		}
		else if(n == 0) {
			E("send return 0");
			break;
		}
		else {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			else if(errno == EINTR) {
				E("errno is EINTR");
			}
			else {
				E("send return error, reason %s", strerror(errno));
				goto failed;
			}
		}
	}

	if(b->offset == b->size) {
		mrpc_buf_reset(b);
	}
	return r;
failed:
	return -1;	
}

int mrpc_send(mrpc_connection_t *c)
{
	return mrpc_send2(c->send_buf, c->fd);
}


int mrpc_recv2(mrpc_buf_t *b, int fd)
{
	int r = 0, n, buf_avail;
	while(1) {
		if(b->len - b->size <= 0) {
			if(mrpc_buf_enlarge(b) < 0) {
				E("enlarge the recv buf error");
				return 0;
			}
		}
		buf_avail = b->len - b->size;

		n = recv(fd, b->buf + b->size, buf_avail, 0);
		D("recv %d bytes",n);

		if(n < 0) {
			if(errno == EAGAIN || errno == EWOULDBLOCK ) {
				return -1;
			}
			else {
				I("read error,errno is %d,reason %s",errno, strerror(errno));
				return 0;
			}
		}

		if(n == 0) {
			I("reset by peer");
			return 0;
		}

		b->size += n;
		r += n;

		if((size_t)n <= buf_avail) {	
			D("break from receive loop, recv enough");	
			break;
		}
	}
	return r;	
}

int mrpc_recv(mrpc_connection_t *c) 
{
	return mrpc_recv2(c->recv_buf, c->fd);
}


//return:
//1 OK, 0 not finish, -1 error
int mrpc_parse(mrpc_buf_t *b, mrpc_message_t *msg, mcp_appbean_proto *proto)
{
	//parse the rpc packet
	if((b->size - b->offset) < 12) {
		//not a full package
		return 0;
	}

	//mask:
	msg->mask = ntohl(*(uint32_t*)(b->buf + b->offset));
	b->offset += 4;
	if(msg->mask != 0x0badbee0 && msg->mask != 0x0badbee1) { 
		E("mask error, %x", msg->mask);
		goto failed;
	}

	//package length
	msg->package_length = ntohl(*(uint32_t*)(b->buf + b->offset));
	b->offset += 4;

	//not a full package
	if(b->size - b->offset < msg->package_length - 8) {
		b->offset -= 8;
		return 0;
	}

	//header length
	msg->header_length = ntohs(*(short*)(b->buf + b->offset));
	b->offset += 2;
	
	//skip the packet options
	b->offset += 2;

	D("unpack the req/resp header");
	struct pbc_slice str = {b->buf + b->offset, msg->header_length};
	int r;
	if(msg->mask == 0x0badbee0) {
		r = pbc_pattern_unpack(rpc_pat_mrpc_request_header, &str, 
				       &msg->h.req_head);
	}
	else {
		r = pbc_pattern_unpack(rpc_pat_mrpc_response_header, &str, 
				       &msg->h.resp_head);
		D("resp_head.code %d", msg->h.resp_head.response_code);
	}
	if(r < 0) {
		E("rpc unpack request header fail");
		goto failed;
	}
	b->offset += msg->header_length;
	D("unpack header finish");

	//body
	int32_t body_len = 0;
	if(msg->mask == 0x0badbee0) {
		body_len = msg->h.req_head.body_length -1;
	}
	else {
		body_len = msg->h.resp_head.body_length -1;
	}
	if(body_len < 0)  {
		body_len = 0;
		D("body_len == 0");
	}
	if(body_len > 0) {
		D("unpack the body, body_len is %d",body_len);
		struct pbc_slice str1 = {b->buf + b->offset, body_len};
		r =pbc_pattern_unpack(rpc_pat_mcp_appbean_proto, &str1, proto);
		if ( r < 0) {
			E("rpc unpack mcp_appbean_proto error");
			return -1;
		}
		b->offset += body_len;
	}
	
	//skip the extentions
	b->offset += msg->package_length - 12 - msg->header_length - body_len;
	return 1;

failed:
	return -1;
}
