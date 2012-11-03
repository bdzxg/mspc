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

	D("request user_context.len %d", args->user_context.len);
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

static int _cli_send(rec_msg_t *msg, mrpc_connection_t *c)
{
	mcp_appbean_proto body;
	get_rpc_arg(&body, msg);
	char *body_buf = malloc(body.content.len + 1024);
	if(!body_buf) {
		E("cannot malloc body_buf");
		return -1;
	}
	struct pbc_slice s = { body_buf, sizeof(*body_buf) };
	int r =	pbc_pattern_pack(rpc_pat_mcp_appbean_proto, &body, &s);
	if(r < 0) {
		E("pack body error");
		goto failed;
	}
	int body_len = body.content.len + 1024 - r;

	mrpc_request_header header;
	header.sequence = ++c->seq;
	header.body_length = body_len + 1; // by protocol we need plus 1
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


static void _send_pending(mrpc_connection_t *c)
{
	UNUSED(c);
//TODO
}

static void mrpc_cli_recv(ev_t *ev, ev_file_item_t *fi)
{
	UNUSED(ev);
	mrpc_connection_t *c = fi->data;
	int err, n;
	socklen_t err_len;

	if(c->conn_status == MRPC_CONN_CONNECTING) {
		getsockopt(c->fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
		if(err == 0) {
			c->conn_status = MRPC_CONN_CONNECTED;
			c->connected = time(NULL);
		}
		else {
			c->conn_status = MRPC_CONN_DISCONNECTED;
			_conn_close(c);
			_conn_free(c);
			//TODO: remove the evnt from ev_main
		}
		return;
	}

	n = _recv(c);
	if(n == 0) {
		goto failed;
	}
	if(n == -1) {
		E("recv return -1");
		return;
	}
	
	mrpc_message_t msg;
	mcp_appbean_proto proto;
	n = _parse(c->recv_buf, &msg, &proto);
	if(n < 0) {
		E("parse error");
		goto failed;
	}
	if(n == 0) {
		return;
	}

	//got the request
	// req = ...
	if(msg.h.resp_head.response_code != 200) {
		//send error response the client
	}
	//remove the request from the map
//free
	
failed:
	_conn_close(c);
	_conn_free(c);
}

static void mrpc_cli_conn_send(ev_t *ev, ev_file_item_t *fi) 
{
	UNUSED(ev);
	D("mrpc_cli_conn_send begins");
	mrpc_connection_t *c = fi->data; 
	if(_send(c) < 0) {
		E("send error");
		//TODO clean ev
		_conn_close(c);
		_conn_free(c);
	}
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

