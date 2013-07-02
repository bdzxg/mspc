#include "proxy.h"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;
extern pxy_settings setting;
static char* PROTOCOL = "MCP/3.0";

void mrpc_cli_ev_in(ev_t *ev, ev_file_item_t *fi);
void mrpc_cli_ev_out(ev_t *ev, ev_file_item_t *fi);
static void _send_pending(mrpc_connection_t *c);

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
                goto failed;
	}

        int k = 1;
        int keepidle = 60 * 8;
        int keepinterval = 10;
        int keepcount = 3;

        if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&k, sizeof(k)) < 0) {
                E("set keepalive failed, reason %s ",strerror(errno));
                goto failed;
        }

        if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle,
                                sizeof(keepidle))) {
                E("set TCP_KEEPIDLE failed, reason %s ", strerror(errno));
                goto failed;
        }

        if (setsockopt(fd, SOL_TCP, TCP_KEEPINTVL,(void*)&keepinterval,
                                sizeof(keepinterval))) {
                E("set TCP_KEEPINTVL failed, reason %s", strerror(errno));
                goto failed;
        }

        if (setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void*)&keepcount,
                                sizeof(keepcount))) {
                E("set TCP_KEEPCNT failed, reason %s", strerror(errno));
                goto failed;
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
                        goto failed;
		}
	}
	else {
		E("address format error");
                goto failed;
	}

	D("begin connect, %s", c->us->uri);
	c->connecting = time(NULL);
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(atoi(r1 + 1));
	addr.sin_addr   = inp;

	int r = connect(fd,(struct sockaddr*)&addr, sizeof(addr));
	if(r < 0) {
		if(errno == EINPROGRESS) {
                        c->conn_status = MRPC_CONN_CONNECTING;
		}
		else {
			E("connect error, %s", strerror(errno));
                        goto failed;
		}
	}
        else if (r == 0) {
 		E("connect succ immediately");
		c->conn_status = MRPC_CONN_CONNECTED;
		c->connected = time(NULL);
                _send_pending(c);
	       
        }
	
	int re = ev_add_file_item(worker->ev, fd, 
			     EV_READABLE | EV_WRITABLE | EPOLLET,
			     c,
			     mrpc_cli_ev_in,
			     mrpc_cli_ev_out);
	if(re < 0) {
		W("add ev error");
                goto failed;
	}
        
	c->fd = fd;
	return 0;

failed:
        close(fd);
        c->conn_status = MRPC_CONN_DISCONNECTED;
        return -1;
}

static mrpc_connection_t* _get_conn(mrpc_us_item_t *us)
{
	int i = 0;
	mrpc_connection_t *c = NULL, *r = NULL;

	
	for(; i < UP_CONN_COUNT; i++) {
		int index = us->current_conn++ % UP_CONN_COUNT;
		c = us->conns[index];

		if(c == NULL) {
			c = mrpc_conn_new(us);
			if(!c) {
				W("cannot malloc mrpc_conn");
			}
			else {
				us->conns[index] = c;
				_connect(c);
			}
			continue;
		}
		
		if(c->conn_status == MRPC_CONN_CONNECTED) {
			r = c;
			break;
		}
		else if(c->conn_status == MRPC_CONN_DISCONNECTED) {
			_connect(c);
		}
	}

	return r;
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
		W("cannot malloc for mrpc_us_item");
		free(up);
		return NULL;
	}

	us->uri = strdup(uri);
	INIT_LIST_HEAD(&us->pending_list);
	
	int i = 0;
	for(; i < UP_CONN_COUNT ; i++) {
		mrpc_connection_t *c = mrpc_conn_new(us);
		if(!c) {
			E("create connection error");
			break;
		}
		us->conns[i] = c;
		_connect(c);
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

        if(r->user_context_len > 0) {
                r->user_context = malloc(r->user_context_len);
                if(!r->user_context) {
                        E("no mem for r->user_content");
                        free(r->body);
                        free(r->option);
                        free(r);
                        return NULL;
                }
                memcpy(r->user_context, msg->user_context, r->user_context_len);
        }

	r->epid = malloc(strlen(msg->epid) + 1);
	if(!r->epid) {
		E("no mem for r->epid");
		free(r->body);
		free(r->option);
                if (r->user_context_len > 0) 
        		free(r->user_context);
		free(r);
		return NULL;
	}
	strcpy(r->epid, msg->epid);

        if (r->user_context_len == 0 || r->user_context == NULL) {
                W("uid %s, cmd %d userctx is null", r->userid, r->cmd);
        }
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

char __endpoint[64] = {0};
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
	}
	else{ 
		args->user_context.len = 0;
		args->user_context.buffer = NULL;
	}

	D("request user_context.len %d, body_len %zu", args->user_context.len, msg->body_len);
	if(msg->body_len > 0){	
		args->content.len = msg->body_len;
		args->content.buffer = msg->body;
	}
	else{
		args->content.len = 0;
		args->content.buffer = NULL;
	}

	args->epid.len = strlen(msg->epid);
	args->epid.buffer = msg->epid;
	args->zip_flag = msg->compress;

	sprintf(__endpoint, "tcp://%s:%d", setting.ip, setting.backend_port);

	args->ip.len = strlen(__endpoint);
	args->ip.buffer = __endpoint;
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
static char _func_name[32] = "requestReceived";

static int _cli_send(rec_msg_t *msg, mrpc_connection_t *c)
{
	mcp_appbean_proto body;
	get_rpc_arg(&body, msg);

	char name[128] = {0};
	if(route_get_app_category_minus_name(msg->cmd, 1, name) > 0){
		body.category_name.len = strlen(name);
		body.category_name.buffer = name;
		D("minus name %s", name);
	}
	else{
		//TODO, we should handle this error
		E("get catetory minus name error");
	}	

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

	header.to_method.buffer = _func_name;
	header.to_method.len = strlen(_func_name);
	
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

void mrpc_cli_ev_in(ev_t *ev, ev_file_item_t *fi)
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
		return;
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
			break;
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
		_stash_req_free(r);
	}

	if(c->recv_buf->offset == c->recv_buf->size) {
		mrpc_buf_reset(c->recv_buf);
	}

	return;
failed:
	I("cli recv failed, address %s, fd %d, %p",c->us->uri, c->fd, c);
	mrpc_conn_close(c);
}


void mrpc_cli_ev_out(ev_t *ev, ev_file_item_t *fi) 
{
	UNUSED(ev);
	mrpc_connection_t *c = fi->data;
	int err = -1;
	socklen_t err_len = sizeof(err);

	if(c->conn_status == MRPC_CONN_CONNECTING) {
		if(getsockopt(c->fd, SOL_SOCKET, SO_ERROR, &err, &err_len) < 0) {
			W("fd %d getsockopt error, reason %s",c->fd,
			  strerror(errno));
			return;
		}

		if(err == 0 || err_len == 0) {
			W("connected!");
			c->conn_status = MRPC_CONN_CONNECTED;
			c->connected = time(NULL);
			_send_pending(c);
		}
		else {
			W("err is %d", err);
			c->conn_status = MRPC_CONN_DISCONNECTED;
			goto failed;
		}
	}

	if(mrpc_send(c) < 0) {
		W("send error, close connection");
		goto failed;
	}
	return;
failed :
	mrpc_conn_close(c);
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
	if(c != NULL)  {
                //try send pending first
                _send_pending(c);
		if(_send_inner(msg, c) < 0) {
			E("send inner return -1");
			return -1;
		}
	}
	else {
		//save to the pending list
		if(us->pend_count >= MRPC_MAX_PENDING) {
			W("max pending");
			return -4;
		}
		rec_msg_t *m = _clone_msg(msg);
		if(!m) {
			W("clone msg error");
			return -1;
		}
		list_append(&m->head, &us->pending_list);
		I("added to the pending list");
		us->pend_count++;
	}
	return 0;
}

void mrpc_ev_after()
{
	//check the connecting connections
	//we should do this in timer
}
