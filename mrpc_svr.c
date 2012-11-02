
//return:
//1 OK, 0 not finish, -1 error
static int _parse(mrpc_buf_t *b, mrpc_message_t *msg);
{
	char *body = NULL;

	//parse the rpc packet
	if((b->size - b->offset) < 12) {
		//not a full package
		return 0;
	}

	//mask:
	msg.mask = ntohl(*(uint32_t*)(b->buf + b->offset));
	b->offset += 4;
	if(msg.mask != 0x0badbee0 && msg.mask != 0x0badbee1) { 
		E("mask error, %x", msg.mask);
		goto failed;
	}

	//package length
	msg.package_length = ntohl(*(uint32_t*)(b->buf + b->offset));
	b->offset += 4;

	//not a full package
	if(b->size - b->offset < msg.package_length - 8) {
		b->offset -= 8;
		return 0;
	}

	//header length
	msg.header_length = ntohs(*(short*)(b->buf + b->offset));
	b->offset += 2;
	
	//skip the packet options
	b->offset += 2;

	D("unpack the req/resp header");
	//request header
	mrpc_request_header reqh;
	mrpc_response_header resph;
	struct pbc_slice str = {b->buf + b->offset, msg.header_length};
	int r;
	if(msg.mask == 0x0badbee0) {
		r = pbc_pattern_unpack(rpc_pat_mrpc_request_header, &str, 
				       &msg.h.req_head);
	}
	else {
		r = pbc_pattern_unpack(rpc_pat_mrpc_response_header, &str, 
				       &msg.h.resp_head);
	}
	if(r < 0) {
		E("rpc unpack request header fail");
		goto failed;
	}
	b->offset += msg.header_length;
	D("unpack header finish");

	//body
	size_t body_len;
	if(msg.mask == 0x0badbee0) {
		body_len = msg.h.req_head.body_length -1 
	}
	else {
		body_len = msg.h.resp_head.body_length -1 
	}
	if(body_len < 0)  {
		body_len = 0;
		D("body_len == 0");
	}
	if(body_len > 0) {
		body = malloc(body_len);
		if(!body) {
			E("malloc body error");
			goto failed;
		}
		memcpy(body, b->buf + b-> offset, body_len);
	
		D("unpack the body, body_len is %lu",body_len);
		//pb deserialize body
		mcp_appbean_proto input;
		struct pbc_slice str1 = {body, body_len};
		r =pbc_pattern_unpack(rpc_pat_mcp_appbean_proto, &str1, &input);
		if ( r < 0) {
			E("rpc unpack mcp_appbean_proto error");
			return -1;
		}
		b->offset += body_len;
	}
	
	//skip the extentions
	b->offset += msg.package_length - 12 - msg.header_length - body_len;
	return 1;

failed:
	if(body != NULL) {
		free(body);
	}
	return -1;
}

static int mrpc_process_client_req(mrpc_connection_t *c)
{
	mrpc_message_t msg;
	mrpc_buf_t *b = c->recv_buf;

	int r = _parse(b, &msg);
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

	char temp[32];
	struct pbc_slice obuf = {temp, 32};
	r = pbc_pattern_pack(rpc_pat_mrpc_response_header, &resp.h.resp_head, &obuf);
	if(r < 0) {
		E("rpc pack response header error");
		goto failed;
	}
	uint32_t resp_size = 12 + 32 - r; //there is no 'body'
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
	sbuf->size += 32 - r;

	int n;
	while(sbuf->size - sbuf->offset > 0) {
		n = send(c->fd, sbuf->buf + sbuf->offset, sbuf->size - sbuf->offset, 0);
		D("mrpc sent %d bytes", n);
		if(n > 0) {
			sbuf->offset += n;
			break;
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
	
	//reset the buf
	if(b->size == b->offset) {
		D("reset the recv buf");
		mrpc_buf_reset(b);
	}
	if(sbuf->size == sbuf->offset) { 
		D("reset the send  buf");
		mrpc_buf_reset(b);
	}

	//TODO: BIZ handler
	return 1;

failed:
	_conn_close(c);
	_conn_free(c);
	return -1;
}

static int _recv(mrpc_connection_t *c) 
{
	int r = 0;
	mrpc_buf_t *b = c->recv_buf;
	size_t buf_avail;
	while(1) {
		buf_avail = b->len - b->size;
		n = recv(fi->fd, b->buf + b->size, buf_avail, 0);
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
			W("socket fd #%d closed by peer",fi->fd);
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

static void mrpc_svr_recv(ev_t *ev,ev_file_item_t *fi)
{
	UNUSED(ev);
	int n;
	mrpc_connection_t *c = fi->data;
	mrpc_buf_t *b = c->recv_buf;

	D("rpc server recv"); 
	n = _recv(c);
	if(n == 0) {
		goto failed;
	}
	if( n == -1) {
		E("recv return -1");
		return;
	}

	int r;
	for(;;) {
		r = mrpc_process_client_req(c);
		if(r < 0) {
			W("process client req error");
			goto failed;
		}
		else if (r == 0) {
			break;
		}
		else {}
	}
	return;

failed:
	W("failed, prepare close!");
	//TODO: clean the connection, and remove the event
	_conn_close(c);
	_conn_free(c);
	return;
}

static int _send(mrpc_connection_t *c)
{
	int r = 0;
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

static void mrpc_svr_send(ev_t *ev, ev_file_item_t *ffi)
{
	UNUSED(ev);
	D("mrpc_svr_send begins");
	mrpc_connection_t *c = ffi->data;
	if(_senda(c) < 0) 
		goto failed;
	return;

failed:
	//TODO clean ev
	_conn_close(c);
	_conn_free(c);
}
