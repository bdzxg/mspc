#include "proxy.h"
#include "mrpc.h"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;
extern pxy_config_t* config;
static 
mrpc_upstreamer_t mrpc_up;
static char* PROTOCOL = "MCP/3.0";

static void mrpc_cli_recv(ev_t *ev, ev_file_item_t *fi);
static void mrpc_cli_conn_send(ev_t *ev, ev_file_item_t *fi);

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

static inline void mrpc_resp_from_req(mrpc_message_t *req, mrpc_message_t *resp)
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



static int _connect(mrpc_connection_t *c) 
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;

	if(fd < 0) {
		E("socket() error, %s",strerror(errno));
		return -1;
	}

	if(setnonblocking(fd) < 0) {
		E("set nonblocking error");
		close(fd);
		return -1;
	}

	char *s = c->us->uri;
	char *r1 = rindex(s, ':');
	char *r2 = rindex(s, '/');
	char r3[16] = {0};
	int addr_len = r1 - r2 -1;
	struct in_addr inp;

	if(addr_len > 0 && addr_len <= 15) {
		strncpy(r3, r2 + 1, addr_len);
		if(inet_aton(r3, &inp) == 0) {
			E("address for aton error");
			return -1;
		}
	}
	else {
		E("address format error");
		return -1;
	}

	D("begin connect, %s", c->us->uri);
	c->connecting = time(NULL);
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(atoi(r1 + 1));
	addr.sin_addr   = inp;

	int r = connect(fd,(struct sockaddr*)&addr, sizeof(addr));
	if(r < 0) {
		if(errno == EINPROGRESS) {
		}
		else {
			E("connect error, %s", strerror(errno));
			close(fd);
			return -1;
		}
	}
	
	ev_file_item_t *fi = ev_file_item_new(fd,
					      c,
					      mrpc_cli_recv,
					      mrpc_cli_conn_send,
					      EV_WRITABLE | EV_READABLE);
	if(!fi) {
		E("create fi error");
		close(fd);
		return -1;
	}

	if(ev_add_file_item(worker->ev, fi) < 0){
		E("add ev error");
		//TODO;
		return -1;
	}
	D("ev item added");

	if(r == 0) {
		D("connect succ immediately");
		c->conn_status = MRPC_CONN_CONNECTED;
		c->connected = time(NULL);
	}
	else {
		c->conn_status = MRPC_CONN_CONNECTING;
	}
	D("connect finish");
	c->fd = fd;
	return 0;
}

static mrpc_connection_t* _conn_new(mrpc_us_item_t* us)
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

static void _conn_close(mrpc_connection_t *c)
{
	close(c->fd);
	c->conn_status = MRPC_CONN_DISCONNECTED;
}

static void _conn_free(mrpc_connection_t *c)
{
	free(c->send_buf);
	free(c->recv_buf);
	list_del(&c->list_us);
	list_del(&c->list_to);
	if(c->us)
		c->us->conn_count--;
	free(c);
}


static int _send(mrpc_connection_t *c)
{
	int r = 0,n;
	mrpc_buf_t *b = c->send_buf;
	while(b->offset < b->size) {
		n = send(c->fd, b->buf + b->offset, b->size - b->offset, 0);
		D("mrpc sent %d bytes", n);
		if(n > 0) {
			b->offset += n;
			r += n;
		}
		if(n == 0) {
			E("send return 0");
			break;
		}
		else {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			else if(errno == EINTR) {

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


static int _recv(mrpc_connection_t *c) 
{
	int r = 0, n;
	mrpc_buf_t *b = c->recv_buf;
	size_t buf_avail;
	while(1) {
		buf_avail = b->len - b->size;
		n = recv(c->fd, b->buf + b->size, buf_avail, 0);
		D("recv %d bytes",n);

		if(n < 0) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				return -1;
			}
			else {
				W("read error,errno is %d",errno);
				return 0;
			}
		}

		if(n == 0) {
			W("socket fd #%d closed by peer",c->fd);
			return 0;
		}

		b->size += n;
		r += n;

		if((size_t)n < buf_avail) {	
			D("break from receive loop, recv enough");	
			break;
		}
		
		//enlarge the recv_buf
		int t = mrpc_buf_enlarge(b);
		if(t < 0) {
			E("recv_buf enlarge error");
			return 0;
		}
	}
	return r;
}


//return:
//1 OK, 0 not finish, -1 error
static int _parse(mrpc_buf_t *b, mrpc_message_t *msg, mcp_appbean_proto *proto)
{
	char *body = NULL;

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


static mrpc_connection_t* _get_conn(mrpc_us_item_t *us)
{
	mrpc_connection_t *c = NULL, *tc, *n;
	time_t now = time(NULL);
	int i = 0;

	D("start for each, us->conn_list %p %p",us->conn_list, us->conn_list.next);
	list_for_each_entry_safe(tc, n, &us->conn_list, list_us) {
	  D("in foreach");
		switch(tc->conn_status)  {
		case MRPC_CONN_DISCONNECTED:
		        D("disconnected");
			_connect(tc);
			break;
		case MRPC_CONN_CONNECTED:
			if(now - tc->connected > MRPC_CONN_DURATION) {
				tc->conn_status = MRPC_CONN_FROZEN;
				list_del(&tc->list_us);
				list_append(&tc->list_us, &us->frozen_list);

				mrpc_connection_t *nc = _conn_new(us);
				if(!nc) {
					E(" cannot new mrpc_conn");
				}
				else {
					list_append(&nc->list_us, &us->conn_list);
					_connect(nc);
				}
			}
			else {
				c = tc;
			}
			break;
		}

		if(c != NULL && i != us->current_conn) {
			i++;
			break;
		}
		i++;
	}

	D("for each quit");
	if(c != NULL) {
		us->current_conn = i - 1 > 0 ? i - 1 : -1;
	}

	D("start for each, us->frozen_list %p %p",us->frozen_list, us->frozen_list.next);
	D("frozen for each");	 
	list_for_each_entry_safe(tc, n, &us->frozen_list, list_us) {
		if(now - tc->connected > MRPC_CONN_DURATION + MRPC_TX_TO * 1.5) {
			list_del(&tc->list_us);
			_conn_close(tc);
			_conn_free(tc);
		}
	}
	D("frozen for each quit, return %p", c);
	return c;
}

static mrpc_us_item_t* _get_us(char *uri)
{
	upstream_t *up = us_get_upstream(upstream_root, uri);
	if(!up) {
		return NULL;
	}
	return (mrpc_us_item_t*)up->data;
}

static mrpc_us_item_t* _us_new(char *uri)
{
	upstream_t *up = calloc(1, sizeof(*up));
	if(!up) {
		E("cannot malloc for upstream_t");
		return NULL;
	}
	up->uri = uri;
	D("uri %s", uri);
	
	mrpc_us_item_t *us = calloc(1, sizeof(*us));
	if(!us) {
		E("cannot malloc for mrpc_us_item");
		free(up);
		return NULL;
	}
	us->uri = uri;
	INIT_LIST_HEAD(&us->conn_list);
	INIT_LIST_HEAD(&us->frozen_list);
	INIT_LIST_HEAD(&us->pending_list);
	
	int i = 0;
	for(; i < 3 ; i++) {
		mrpc_connection_t *c = _conn_new(us);
		if(!c) {
			E("create connection error");
			break;
		}
		list_append(&c->list_us, &us->conn_list);
	}

	if(us_add_upstream(upstream_root, up) < 0){
		E("cannot add the up to the RBTree");
		free(up);
		free(us);
		return NULL;
	}

	up->data =us;
	return us;
}

static rec_msg_t* _clone_msg(rec_msg_t *msg)
{
	rec_msg_t *r = malloc(sizeof(*r));
	if(!r) {
		E("no memory for rec_msg_t");
		return NULL;
	}
	memcpy(r, msg, sizeof(*r));

	if(r->option_len > 0) {
	       r->option = malloc(r->option_len);
	       D("opt len %d", r->option_len);
	       if(!r->option) {
		 E("no memory for r->option");
		 free(r);
		 return NULL;
	       }
	       memcpy(r->option, msg->option, r->option_len);
	}

	r->body = malloc(r->body_len);
	if(!r->body) {
		E("no mem for r->body");
		free(r->option);	       
		free(r);
                return NULL;
	}
	memcpy(r->body, msg->body, r->body_len);

	r->user_context = malloc(r->user_context_len);
	if(!r->user_context) {
		E("no mem for r->user_content");
		free(r->body);
		free(r->option);
		free(r);
		return NULL;
	}
	memcpy(r->user_context, msg->user_context, r->user_context_len);

	r->epid = malloc(strlen(msg->epid) + 1);
	if(!r->epid) {
		E("no mem for r->epid");
		free(r->body);
		free(r->option);
		free(r->user_context);
		free(r);
		return NULL;
	}
	strcpy(r->epid, msg->epid);

	return r;
}

#include "mrpc_svr.c"
#include "mrpc_cli.c"

static void mrpc_mrpc_up_accept(ev_t *ev, ev_file_item_t *ffi)
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
			E("set nonblocking error"); return;
		}
		c = _conn_new(NULL);
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
		ev_add_file_item(worker->ev,fi);
	}	
	else {
		E("accept error %s", strerror(errno));
	}
	return;

failed:
	if(c) {
		_conn_close(c);
		_conn_free(c);
		if(c->send_buf) {
			free(c->send_buf);
		}
		if(c->recv_buf) {
			free(c->recv_buf);
		}
		free(c);
	}
}


int mrpc_start()
{
	ev_file_item_t* fi ;
	int fd = mrpc_up.listen_fd;

	if(listen(fd, 1024) < 0){
		E("rpc listen error");
		return -1;
	}

	fi = ev_file_item_new(fd, worker, mrpc_mrpc_up_accept, NULL, EV_READABLE);
	if(!fi){
		E("create ev for listen fd error");
		goto start_failed;
	}
	ev_add_file_item(worker->ev,fi);
	return 0;

start_failed:
	return -1;
}


int mrpc_init()
{
	D("mrpc init begins");

	struct sockaddr_in addr1;

	mrpc_up.listen_fd = -1;
	INIT_LIST_HEAD(&mrpc_up.conn_list);

	mrpc_up.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(mrpc_up.listen_fd < 0)  {
		E("create rpc listen fd error");
		return -1;
	}
	int reuse = 1;
	setsockopt(mrpc_up.listen_fd , SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	
	if(setnonblocking(mrpc_up.listen_fd) < 0){
		E("set nonblocling error");
		return -1;
	}

	addr1.sin_family = AF_INET;
	addr1.sin_port = htons(config->backend_port);
	addr1.sin_addr.s_addr = 0;
	
	if(bind(mrpc_up.listen_fd, (struct sockaddr*)&addr1, sizeof(addr1)) < 0){
		E("bind error");
		return -1;
	}
	D("mrpc init finish, port %d", config->backend_port);
	return 0;
}
