#include "proxy.h"
#include "mrpc.h"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;
extern pxy_config_t* config;
mrpc_mrpc_up_t mrpc_up;

static void mrpc_buf_free(mrpc_buf_t *buf) 
{
	free(buf->buf);
	free(buf);
}

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

#include "mrpc_cli.c"

static int _connect(mrpc_connection_t *c) 
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr;

	if(fd < 0) {
		E("socket() error, %s",strerror(errno));
		return -1;
	}

	if(setnonblocking(fd) < 0) {
		E("set nonblocking error");
		close(fd);
		reutrn -1;
	}

	D("begin connect");
	c->connecting = time(NULL);
	int r = connect(fd,(struct sockarrd*)&addr, sizeof(addr));
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
					      mrpc_cli_conn_qsend,
					      EV_WRITABLE | EV_READABLE | EPOLLET);
	if(!fi) {
		E("create fi error");
		close(fd);
		return -1;
	}

	if(r == 0) {
		D("connect succ immediately");
		c->conn_status = MRPC_CONN_CONNECTED;
		c->connected = time(NULL);
	}
	else {
		c->conn_status = MRPC_CONN_CONNECTING;
		//add to connecting list
		list_append(&c->list_to, &mrpc_up->conn_list);
	}

	c->fd = fd;
	return 0;
}

static mrpc_connection_t* _conn_new()
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
	c->conn_status = MRPC_CONN_DISCONNECTED;

	INIT_LIST_HEAD(&c->list_us);
	INIT_LIST_HEAD(&c->list_to);
	
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

static void mrpc_connection_free(mrpc_connection_t *c) 
{
	mrpc_buf_free(c->send_buf);
	mrpc_buf_free(c->recv_buf);
	free(c);
}

#include "mrpc_svr.c"

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
		c = _conn_new();
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

static mrpc_connection_t* _get_conn(mrpc_us_item_t *us)
{
	mrpc_connection_t *c = NULL, *tc, *n;
	time_t now = time(NULL);
	int loop = 1;
	int i = 0;

	list_for_each_entry_safe(tc, n, &us->conn_list, list_us) {
		switch(tc->conn_status)  {
		case MRPC_CONN_DISCONNECTED:
			_connect(tc);
			break;
		case MRPC_CONN_CONNECTED:
			if(now - tc->connected > MRPC_CONN_DURATION) {
				tc->status = MRPC_CONN_FROZEN;
				list_del(&tc->list_us);
				list_append(&tc->list_us, &us->frozen_list);

				mrpc_connection_t *nc = _conn_new();
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

	if(c != NULL) {
		us->current_conn = i - 1 > 0 ? i - 1 : -1;
	}

	list_for_each_entry_safe(tc, n, &us->frozen_list, list_us) {
		if(now - tc->connected > MRPC_CONN_DURATION + MRPC_TX_TO * 1.5) {
			list_del(&tc->us_list);
			_conn_close(tc);
			_conn_free(tc);
		}
	}
	return c;
}

static mrpc_us_item_t* _get_us(char *uri)
{
	upstream_t *up = us_get_upstream(&upstream_root, uri);
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
	
	mrpc_us_item_t *us = calloc(1, sizeof(*us));
	if(!us) {
		E("cannot malloc for mrpc_us_item");
		free(up);
		return NULL;
	}
	
	if(us_add_upstream(&upstream_root, up) < 0){
		E("cannot add the up to the RBTree");
		free(up);
		free(us);
		return NULL;
	}

	up->data =us;
	return us;
}


//error code :
// 0 OK
// -1 send failed
// -2 connection timeout 
// -3 no us_item
// -4 max pending 
int mrpc_us_send(rec_msg_t *msg)
{
	mrpc_us_item_t *us;
	mrpc_connection_t *c;

	//1. get the us_item by address
	us = _get_us(msg);
	if(!us) {
		us = _us_new();
		if(!us) {
			E("cannnot create us item");
			return -3;
		}
	}
	
	//2. get the connection from the us_item
	c = _get_conn(us);
	if(c != NULL)  {
		//send it
		mrpc_message_t *req = _create_req (msg);
		if(!req) {
			return -1;
		}
		if(_cli_send(req, c) < 0) {
			return -1;
		}
		list_append(&req->head, &mrpc_up->sent_list);
	}
	else {
		//save to the pending list
		if(us->pend_count >= MRPC_MAX_PENDING) {
			E("max pending");
			return -4;
		}
		list_append(&req->head, &us->pending_list);
	}
	reutrn 0;
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
