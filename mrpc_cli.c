
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

static char* get_compact_uri(rec_msg_t* msg)
{
	char* ret;
	ret = malloc(36);
	if(ret == NULL)
		return ret;
	sprintf(ret, "id:%d;p=%d;", msg->userid,msg->logic_pool_id);	
	return ret;
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
static char _method_name[14]  = "mcore-Reg2V501";

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
	header.to_method.buffer = _method_name;
	header.to_method.len = sizeof(_method_name);
	//TODO: service method
	
	char header_buf[128];
	struct pbc_slice s2 = {header_buf, 128};
	r = pbc_pattern_pack(rpc_pat_mrpc_request_header, &header, &s2);
	if(r < 0) {
		E("pack header error");
		goto failed;
	}
	int head_len = 128 - r;
	size_t pkg_len = 12 + body_len + head_len;
	while( c->send_buf->len - c->send_buf->size < pkg_len) {
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

	free(body_buf);
	
	//TODO send the buffer
	if(_send(c) < 0) {
		goto failed;
	}
	//TODO: save the request to the sent_map
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
	}
	else {
		r->seq = c->seq - 1;
		_map_add(c, r);
	}
	//TODO: add a new timer to watch the timeout

	//save to the sent map
	if(_map_add(c, r) < 0) {
		E("save the stash_req failed");
		_stash_req_free(r);
	}

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

		if(iter->option_len > 0) 
		  free(iter->option);
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

	n = _recv(c);
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
		n = _parse(c->recv_buf, &msg, &proto);
		if(n < 0) {
			E("parse error");
			goto failed;
		}
		if(n == 0) {
			return;
		}

		//find the sent request
		uint32_t seq = msg.h.resp_head.sequence;
		mrpc_stash_req_t *r = _map_remove(c, seq);
		if(!r) {
			E("cannot find the seq %u sent request", seq);
			continue;
		}
		if(msg.h.resp_head.response_code != 200) {
			//TODO find the agent, and send the error response
		}
		free(r);
	}

	if(c->recv_buf->offset == c->recv_buf->size) {
		mrpc_buf_reset(c->recv_buf);
	}
failed:
	_conn_close(c);
	_conn_free(c);
}


void mrpc_cli_ev_out(ev_t *ev, ev_file_item_t *fi) 
{
	UNUSED(ev);
	mrpc_connection_t *c = fi->data;
	int err;
	socklen_t err_len;

	D("mrpc_cli_ev_out begins");
	D("status %d, %d", c->conn_status, MRPC_CONN_CONNECTING);
	if(c->conn_status == MRPC_CONN_CONNECTING) {
		D("connecting");
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
			_conn_close(c);
			_conn_free(c);
			return;
			//TODO: remove the evnt from ev_main
		}
	}

	if(_send(c) < 0) {
		E("send error");
		//TODO clean ev
		_conn_close(c);
		_conn_free(c);
	}
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
	list_head_t *head = &mrpc_up.conn_list;
	mrpc_connection_t *c, *n;
	time_t now = time(NULL);

	list_for_each_entry_safe(c, n, head, list_to) {
		if(now - c->connecting > MRPC_CONNECT_TO) {
			list_del(&c->list_to);
			_conn_close(c);
			_conn_free(c);
		}
	}
}
