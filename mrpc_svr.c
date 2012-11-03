static int mrpc_process_client_req(mrpc_connection_t *c)
{
	mrpc_message_t msg;
	mrpc_buf_t *b = c->recv_buf;
	mcp_appbean_proto *proto;
	proto = malloc(sizeof(*proto));
	if(!proto) {
		E("no mem for proto");
		return -1;
	}

	int r = _parse(b, &msg, proto);
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

	//TODO: BIZ handler
	

	free(proto);
	//reset the buf
	if(b->size == b->offset) {
		D("reset the recv buf");
		mrpc_buf_reset(b);
	}
	if(sbuf->size == sbuf->offset) { 
		D("reset the send  buf");
		mrpc_buf_reset(sbuf);
	}

	return 1;

failed:
	free(proto);
	_conn_close(c);
	_conn_free(c);
	return -1;
}

static void mrpc_svr_recv(ev_t *ev,ev_file_item_t *fi)
{
	UNUSED(ev);
	int n;
	mrpc_connection_t *c = fi->data;
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

static void mrpc_svr_send(ev_t *ev, ev_file_item_t *ffi)
{
	UNUSED(ev);
	D("mrpc_svr_send begins");
	mrpc_connection_t *c = ffi->data;
	if(_send(c) < 0) 
		goto failed;
	return;

failed:
	//TODO clean ev
	_conn_close(c);
	_conn_free(c);
}
