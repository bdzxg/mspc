#include "proxy.h"
#include "mrpc.h"

static buffer_t* 
get_buf_for_write(rpc_upstreamer_connection_t *c)
{
	buffer_t *b = buffer_fetch(worker->buf_pool,worker->buf_data_pool);

	if(b == NULL) {
		E("no buf available"); 
		return NULL;
	}

	if(c->recv_buf == NULL) {
		c->send_buf = b;
	}
	else {
		buffer_append(b,c->send_buf);
	}

	return b;
}


static buffer_t* 
get_buf_for_read(rpc_upstreamer_connection_t *c)
{
	buffer_t *b = buffer_fetch(worker->buf_pool,worker->buf_data_pool);

	if(b == NULL) {
		E("no buf available"); 
		return NULL;
	}

	if(c->recv_buf == NULL) {
		c->recv_buf = b;
	}
	else {
		buffer_append(b,c->recv_buf);
	}

	return b;
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

static int rpc_process_client_req(rpc_upstreamer_connection_t *c)
{
	int index = 0, buf_len = 0;
	char *c1,*c2,*c3,*c4;
	rpc_message_t msg;

	//parse the rpc packet
	if((buf_len = buffer_length(c->recv_buf)) < 12) {
		//not a full package
		return 0;
	}

	//mask:
	c1 = buffer_read(c->recv_buf, index++);
	c2 = buffer_read(c->recv_buf, index++);
	c3 = buffer_read(c->recv_buf, index++);
	c4 = buffer_read(c->recv_buf, index++);

	msg.mask = (*c1 << 24 ) | (*c2 << 16) | (*c3 << 8) | *c4;

	//package length
	c1 = buffer_read(c->recv_buf, index++);
	c2 = buffer_read(c->recv_buf, index++);
	c3 = buffer_read(c->recv_buf, index++);
	c4 = buffer_read(c->recv_buf, index++);

	msg.package_length = (*c1 << 24 ) | (*c2 << 16) | (*c3 << 8) | *c4;

	if(buf_len < msg.package_length) {
		//not a full package
		return 0;
	}

	//header length
	c1 = buffer_read(c->recv_buf, index++);
	c2 = buffer_read(c->recv_buf, index++);

	msg.package_length = (*c1 << 8 ) | (*c2 );

	//skip the packet options
	index += 2;

	//body
	int body_len = msg.package_length - msg.header_length;
	char *body = calloc(1 , body_len);
	int i;
	for (i = 0; i < body_len; i++) {
		body[i] = buffer_read(c->recv_buf, 12 + msg.header_length);
	}

	//pb deserialize
	McpAppBeanProto input;
	rpc_pb_string str = {body, (size_t)body_len};
	int r = rpc_pb_pattern_unpack(rpc_pat_mcpappbeanproto, &str, &input);

	if ( r < 0) {
		//TODO: handle error
		rpc_return_error(c, RPC_CODE_INVALID_REQUEST_ARGS, "input decode failed!");
		return;
	}

	//rpc return:
	//TODO:
	retval output;
	output.option.len = input.Protocol.len;
	output.option.buffer = input.Protocol.buffer;

	rpc_pb_string str_out;
	int outsz = 256;  
	str_out.buffer = rpc_connection_alloc(c, outsz);
	str_out.len = outsz;
	r = rpc_pb_pattern_pack(rpc_pat_retval, &output, &str_out);

	if (r >= 0) {
 		 rpc_return(c, str_out.buffer, str_out.len);
	}
	else {
		rpc_return_error(c, RPC_CODE_SERVER_ERROR, "output encode failed!");
	}
	
	//send the server request to the client
	pxy_agent_t* a = NULL;
	a = get_agent(input.Epid.buffer);
	if(a == NULL) {
		if(input.Epid.buffer != NULL) {
			W("BN cann't find epid %s connection!!!", msg.epid);
		}
		//TODO: handle error
		return;
	}

	msg.seq = a->bn_seq++;
	process_bn(&msg, a);
	W("BN epid %s!", msg.epid);
}

void rpc_upstreamer_server_recv(ev_t *ev,ev_file_item_t *fi)
{
	UNUSED(ev);
	int n;
	buffer_t *b;
	rpc_upstreamer_connection_t *c = fi->data;


	D("rpc server recv"); 
	if(!c)
	{
		W("fd has no rpc_connection ,ev->data is NULL,close the fd");
		close(fi->fd); return;
	}

	while(1) {
		b = get_buf_for_read(agent);
		if(b == NULL) {
			E("no buf for read");
			goto failed;
		}

		n = recv(fi->fd,b->data,BUFFER_SIZE,0);
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

		b->len = n;
		agent->buf_len += n;

		if(n < BUFFER_SIZE) {	
			D("break from receive loop, n > BUFFER_SIZE");	
			break;
		}
	}

	D("buffer-len %zu", agent->buf_len);
	if(rpc_process_client_req(c) < 0) {
		W("rpc: process client request failed");
		goto failed;
	}
	return;

failed:
	W("failed, prepare close!");
	//TODO: clean the connection
	close(fi-->fd);
	free(fi);
	return;
}

void rpc_upstreamer_accept (ev_t *ev, ev_file_item_t *ffi)
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
		c->fd = f;

		fi = ev_file_item_new(f,
				c,
				rpc_upstreamer_server_recv,
				rpc_upstreamer_server_write,
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


int rpc_upstreamer_start()
{
	ev_file_item_t* fi ;
	int fd = upstreamer.fd;

	if(listen(fd, 1024) < 0){
		E("rpc listen error");
		return -1;
	}

	fi = ev_file_item_new(fd, worker, rpc_upstreamer_accept, NULL, EV_READABLE);
	if(!fi){
		E("create ev for listen fd error");
		goto start_failed;
	}
	ev_add_file_item(worker->ev,fi);


start_failed:
	return -1;
}


int rpc_upstreamer_init()
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



