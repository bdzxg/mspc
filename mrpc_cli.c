#include "proxy.h"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;
extern pxy_config_t* config;
static mrpc_upstreamer_t mrpc_up;
static char* PROTOCOL = "MCP/3.0";

int mrpc_cli_ev_in(ev_t *ev, ev_file_item_t *fi);
int mrpc_cli_ev_out(ev_t *ev, ev_file_item_t *fi);


//TODO handle the error when we cannot connect the backend
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
	D("str 2");

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
					      mrpc_cli_ev_in,
					      mrpc_cli_ev_out,
					      EV_WRITABLE | EV_READABLE | EPOLLET);
	if(!fi) {
		E("create fi error");
		close(fd);
		return -1;
	}

	if(ev_add_file_item(worker->ev, fi) < 0){
		E("add ev error");
		ev_file_free(fi);
		return -1;
	}
	D("ev item added");
	c->event = fi;

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

static mrpc_connection_t* _get_conn(mrpc_us_item_t *us)
{
	mrpc_connection_t *c = NULL, *tc, *n;
	time_t now = time(NULL);
	int i = 0;
	
	list_for_each_entry_safe(tc, n, &us->conn_list, list_us) {
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

				mrpc_connection_t *nc = mrpc_conn_new(us);
				if(!nc) {
					E(" cannot new mrpc_conn");
				}
				else {
					list_append(&nc->list_us, &us->conn_list);
					W("connect new connection for frozen");
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
			//list_del(&tc->list_us);
			mrpc_conn_close(tc);
			mrpc_conn_free(tc);
		}
	}
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
	up->uri = strdup(uri);
	D("uri %s", uri);
	
	mrpc_us_item_t *us = calloc(1, sizeof(*us));
	if(!us) {
		E("cannot malloc for mrpc_us_item");
		free(up);
		return NULL;
	}

	us->uri = strdup(uri);
	INIT_LIST_HEAD(&us->conn_list);
	INIT_LIST_HEAD(&us->frozen_list);
	INIT_LIST_HEAD(&us->pending_list);
	
	int i = 0;
	for(; i < 3 ; i++) {
		mrpc_connection_t *c = mrpc_conn_new(us);
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

/*
  TODO: we ignore option now
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
*/

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

//TODO 
//Handle rpc exception

static int _map_add(mrpc_connection_t *c, mrpc_stash_req_t *req)
{
	struct rb_root *root = &c->root;
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	while(*new) {
		mrpc_stash_req_t *data = rb_entry(*new, struct mrpc_stash_req_s, rbnode);
		int result = data->seq - req->seq;
		parent = *new;
		if(result < 0)
			new = &((*new)->rb_left);
		else if(result > 0)
			new = &((*new)->rb_right);
		else 
			return -1;
	}

	rb_link_node(&req->rbnode,parent,new);
	rb_insert_color(&req->rbnode,root);
	return 1;

}

static mrpc_stash_req_t* _map_get(mrpc_connection_t *c, uint32_t seq)
{
	struct rb_root *root = &c->root;
	struct rb_node *node = root->rb_node;

	while(node) {
		mrpc_stash_req_t *q = rb_entry(node, struct mrpc_stash_req_s, rbnode);
		int result = q->seq - seq;

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else 
			return q;
	}

	return NULL;
}


static mrpc_stash_req_t* _map_remove(mrpc_connection_t *c, uint32_t seq)
{
	mrpc_stash_req_t *r = _map_get(c, seq);
	if(r) {
		rb_erase(&r->rbnode, &c->root);
	}
	return r;
}

char __compact_uri[64] = {0};
static char* get_compact_uri(rec_msg_t* msg)
{
	sprintf(__compact_uri, "id:%d;p=%d;", msg->userid,msg->logic_pool_id);	
	return __compact_uri;
}

static void get_rpc_arg(mcp_appbean_proto* args, rec_msg_t* msg)
{
	args->protocol.len = strlen(PROTOCOL);
	args->protocol.buffer = PROTOCOL;
	args->cmd = msg->cmd;
	char* uri = get_compact_uri(msg);
	args->compact_uri.len = strlen(uri);
	args->compact_uri.buffer = uri;
	args->user_id = msg->userid;
	args->sequence = msg->seq;
	args->opt = msg->format;

	if(msg->user_context_len > 0){
		args->user_context.len = msg->user_context_len;
		args->user_context.buffer = msg->user_context;
	}else{ args->user_context.len = 0;  args->user_context.buffer = NULL;}

	D("request user_context.len %d, body_len %d", args->user_context.len, msg->body_len);
	if(msg->body_len > 0){	
		args->content.len = msg->body_len;
		args->content.buffer = msg->body;
	}
	else{
		args->content.len = 0; args->content.buffer = NULL;
	}

	args->epid.len = strlen(msg->epid);
	args->epid.buffer = msg->epid;
	args->zip_flag = msg->compress;
}	


static mrpc_stash_req_t* _stash_req_new(rec_msg_t *msg)
{
	mrpc_stash_req_t *r = malloc(sizeof(*r));
	if(!r) {
		E("no mem for mrpc_stash_req");
		return NULL;
	}
	r->epid = malloc(strlen(msg->epid) + 1);
	if(!r->epid) {
		E("no mem for r->epid");
		free(r);
		return NULL;
	}
	
	r->user_id = msg->userid;
	r->mcp_seq = msg->seq;
	strcpy(r->epid, msg->epid);

	return r;
}

static void  _stash_req_free(mrpc_stash_req_t *r) 
{
	free(r->epid);
	free(r);
}

static char _service_name[3] = "MCP";
static int _cli_send(rec_msg_t *msg, mrpc_connection_t *c)
{
	mcp_appbean_proto body;
	get_rpc_arg(&body, msg);
	size_t buf_len = body.content.len + 1024;
	char *body_buf = malloc(buf_len);
	if(!body_buf) {
		E("cannot malloc body_buf");
		return -1;
	}
	struct pbc_slice s = { body_buf, buf_len };
	D("the buf size is %lu", buf_len);
	int r =	pbc_pattern_pack(rpc_pat_mcp_appbean_proto, &body, &s);
	if(r < 0) {
		E("pack body error");
		goto failed;
	}

	int body_len = buf_len - r;
	D("body_len is %d", body_len);
	mrpc_request_header header;
	memset(&header, 0, sizeof(header));
	header.sequence = ++c->seq;
	header.body_length = body_len + 1; // by protocol we need plus 1
	header.to_service.buffer = _service_name;
	header.to_service.len = sizeof(_service_name);

	//TODO to load from the config
	char *fname = get_cmd_func_name(msg->cmd);
	if(fname == NULL) {
		W("no func name for cmd %d", msg->cmd);
		goto failed;
	}
	D("the funcname is %s", fname);
	header.to_method.buffer = fname;
	header.to_method.len = strlen(fname);
	
	char header_buf[128];
	struct pbc_slice s2 = {header_buf, 128};
	r = pbc_pattern_pack(rpc_pat_mrpc_request_header, &header, &s2);
	if(r < 0) {
		E("pack header error");
		goto failed;
	}
	int head_len = 128 - r;
	size_t pkg_len = 12 + body_len + head_len;
	while(c->send_buf->len - c->send_buf->size < pkg_len) {
		if(mrpc_buf_enlarge(c->send_buf) < 0) {
			E("enlarge error");
			goto failed;
		}
	}

	*(uint32_t*)(c->send_buf->buf + c->send_buf->size) = htonl(0x0badbee0);
	c->send_buf->size += 4;
	*(uint32_t*)(c->send_buf->buf + c->send_buf->size) = htonl(pkg_len);
	c->send_buf->size += 4;
	*(uint16_t*)(c->send_buf->buf + c->send_buf->size) = htons((short)head_len);
	c->send_buf->size += 2;
	*(uint16_t*)(c->send_buf->buf + c->send_buf->size) = 0;
	c->send_buf->size += 2;

	memcpy(c->send_buf->buf + c->send_buf->size, header_buf, head_len);
	c->send_buf->size += head_len;
	
	memcpy(c->send_buf->buf + c->send_buf->size, body_buf, body_len);
	c->send_buf->size += body_len;

	if(mrpc_send(c) < 0) {
		goto failed;
	}
	free(body_buf);
	return 0;
failed:
	free(body_buf);
	return -1;
}


static int _send_inner(rec_msg_t *msg, mrpc_connection_t *c) 
{
	//send it
	if(_cli_send(msg, c) < 0) {
		E("cli_send return < 0");
		return -1;
	}
	mrpc_stash_req_t *r = _stash_req_new(msg);
	if(!r) {
		E("no mem for mrpc_stash_req_t");
		return -1;
	}

	r->seq = c->seq;
	//save to the sent map
	if(_map_add(c, r) < 0) {
		E("save the stash_req failed");
		_stash_req_free(r);
	}
	D("seq %d stashed", r->seq);
	return 0;
}

static void _send_pending(mrpc_connection_t *c)
{
	rec_msg_t *iter, *n;
	list_for_each_entry_safe(iter, n, &c->us->pending_list, head) {
		if(_send_inner(iter, c) < 0) {
			E("send pending request error");
			break;
		}
		list_del(&iter->head);

/*
  TODO
		if(iter->option_len > 0) 
		  free(iter->option);
*/
		if(iter->user_context_len > 0)
		  free(iter->user_context);
		if(iter->body_len > 0)
		  free(iter->body);

		free(iter->epid);
		free(iter);
	}
}

int mrpc_cli_ev_in(ev_t *ev, ev_file_item_t *fi)
{
	UNUSED(ev);
	mrpc_connection_t *c = fi->data;
	int n;
	D("mrpc_cli_ev_in");

	n = mrpc_recv(c);
	if(n == 0) {
		goto failed;
	}
	if(n == -1) {
		E("recv return -1, wired");
		return 0;
	}
	
	mrpc_message_t msg;
	mcp_appbean_proto proto;
	
	for( ;; ) {
		n = mrpc_parse(c->recv_buf, &msg, &proto);
		if(n < 0) {
			E("parse error");
			goto failed;
		}
		if(n == 0) {
			return 0;
		}

		//find the sent request
		uint32_t seq = msg.h.resp_head.sequence;
		mrpc_stash_req_t *r = _map_remove(c, seq);
		if(!r) {
			E("cannot find the seq %u sent request", seq);
			continue;
		}
		if(msg.h.resp_head.response_code != 200) {
		        W("resp code %d", msg.h.resp_head.response_code);
			//TODO find the agent, and send the error response
		}
		free(r);
	}

	if(c->recv_buf->offset == c->recv_buf->size) {
		mrpc_buf_reset(c->recv_buf);
	}
failed:
	mrpc_conn_close(c);
	return -1;
}


int mrpc_cli_ev_out(ev_t *ev, ev_file_item_t *fi) 
{
	UNUSED(ev);
	mrpc_connection_t *c = fi->data;
	int err;
	socklen_t err_len;

	if(c->conn_status == MRPC_CONN_CONNECTING) {
		getsockopt(c->fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
		if(err != 0) {
			D("connected!");
			c->conn_status = MRPC_CONN_CONNECTED;
			c->connected = time(NULL);
			_send_pending(c);
		}
		else {
			D("connect error");
			c->conn_status = MRPC_CONN_DISCONNECTED;
			goto failed;
		}
	}

	if(mrpc_send(c) < 0) {
		E("send error, close connection");
		mrpc_conn_close(c);
	}

	return 0;
failed :
	mrpc_conn_close(c);
	return -1;
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
	us = _get_us(msg->uri);
	if(!us) {
		W("new upstream for %s", msg->uri);
		us = _us_new(msg->uri);
		if(!us) {
			E("cannnot create us item");
			return -3;
		}
	}
	
	//2. get the connection from the us_item
	c = _get_conn(us);
	D("get connection %p", c);
	if(c != NULL)  {
		if(_send_inner(msg, c) < 0) {
			E("send inner return -1");
			return -1;
		}
	}
	else {
		//save to the pending list
		if(us->pend_count >= MRPC_MAX_PENDING) {
			E("max pending");
			return -4;
		}
		D("begin clone");
		rec_msg_t *m = _clone_msg(msg);
		D("clone finish");
		if(!m) {
			E("clone msg error");
			return -1;
		}
		list_append(&m->head, &us->pending_list);
	}
	return 0;
}

void mrpc_ev_after()
{
	//check the connecting connections
	//we should do this in timer
}
