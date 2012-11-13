#include "proxy.h"
#include "agent.h"
#include "include/zookeeper.h"
#include <pthread.h>
#include "route.h"
#include "ClientHelper.c"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;

size_t packet_len;

static char OVERTIME[3] = {8, 248, 3};// 504
static char SERVERERROR[3] = {8, 244, 3};// 500
static char REQERROR[3] = {8, 144, 3};// 400
static char NOTEXIST[3] = {8, 155, 3};// 411

static int agent_to_beans(pxy_agent_t *a, rec_msg_t* msg, int msp_unreg);

void char_to_int(int*v, buffer_t* buf, int* off, int len)
{
	*v = 0;
	char* t = (char*)v;
	int i;
	for(i = 0 ; i < len; i++) {
		*(t+i)= *buffer_read(buf, *off);
		(*off)++;
	}
}

int worker_insert_agent(pxy_agent_t *agent)
{
	int ret = 0;
	pthread_mutex_lock (&worker->mutex);
	ret = map_insert(&worker->root,agent);
	pthread_mutex_unlock(&worker->mutex);
	return ret;
}

void worker_remove_agent(pxy_agent_t *agent)
{
	if(agent->epid != NULL) {
		pthread_mutex_lock(&worker->mutex);	
		map_remove(&worker->root, agent->epid);
		pthread_mutex_unlock(&worker->mutex);
	}
}

void worker_insert_reg3(reg3_t* r3)
{
	pthread_mutex_lock(&worker->r3_mutex);
	map_insert_reg3(&worker->r3_root, r3);	
	pthread_mutex_unlock(&worker->r3_mutex);
}

reg3_t* worker_remove_reg3(char* key)
{
	pthread_mutex_lock(&worker->r3_mutex);
	reg3_t* r3 = map_remove_reg3(&worker->r3_root, key);
	pthread_mutex_unlock(&worker->r3_mutex);
	return r3;
}

void msp_send_unreg(reg3_t* a)
{
	if(a->user_context!= NULL) {
		rec_msg_t msg;
		msg.cmd = 103;
		msg.userid = a->user_id;
		msg.logic_pool_id = a->logic_pool_id;
		msg.seq = 100001;
		msg.format = 0;
		msg.user_context_len = a->user_context_len;
		msg.user_context = a->user_context;
		msg.body_len = 0;
		msg.epid = a->epid;
		msg.compress = 0;
		agent_to_beans(NULL, &msg, 1);
	}
}

void msp_send_req_to_bean(pxy_agent_t* a, int cmd, int bodylen, char* body)
{
	if(a->user_ctx != NULL) {
		rec_msg_t msg;
		msg.cmd = cmd;
		msg.userid = a->user_id;
		msg.logic_pool_id = a->logic_pool_id;
		msg.seq = 100000;
		msg.format = 0;
		msg.user_context_len = a->user_ctx_len;
		msg.user_context = a->user_ctx;
		msg.body_len = bodylen;
		msg.body = body;
		msg.epid = a->epid;
		msg.compress = 0;
		agent_to_beans(a, &msg, 1);
	}
}

void release_reg3(reg3_t* r3)
{
	free(r3->epid);
	free(r3->user_context);
	free(r3);
}

//TODO change the return type to int?
void send_to_client(pxy_agent_t* a, char* data, size_t len )
{
	D("prepare to send %lu bytes data", len);
	//1. prepare the send buf
	if(!a->send_buf) {
		a->send_buf = malloc(len);
		if(!a->send_buf) {
			E("cannot malloc send_buf");
			return;
		}
		a->send_buf_len = len;
	}

	size_t available = a->send_buf_len - a->send_buf_offset;

	if(len > available) {
		size_t newlen = a->send_buf_len + (len - available);
		void* tmp = realloc(a->send_buf, newlen);
		if(!tmp) {
			E("realloc send buf error");
			return;
		}
		a->send_buf = tmp;
		a->send_buf_len = newlen;
	}

	//2. copy to send buf
	memcpy(a->send_buf + a->send_buf_offset, data, len);

	//3. try to send 
	int n  = 0;

	for (;;) {
		int to_send = a->send_buf_len - a->send_buf_offset;
		char tmp[102400] = {0};
		to_hex(a->send_buf, a->send_buf_len, tmp);
		D("packet is %s" ,tmp);
		n = send(a->fd, a->send_buf + a->send_buf_offset , to_send, 0);

		if(n > 0) {
			D("a->fd %d, send %d/%d bytes", a->fd,  n, to_send);
			a->downflow += n;

			if(n < to_send) {
				a->send_buf_offset += n;
				return;
			}

			free(a->send_buf);
			a->send_buf = NULL;
			a->send_buf_len = 0;
			a->send_buf_offset = 0;
			D("send_buf freed");
			return;
		}


		if(n ==  0) {
			D("send return 0");
			return;
		}

		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		}
		else if(errno == EINTR) {

		}
		else {
			E("send return error, reason %s", strerror(errno));
			return;
		}
	}
	return;
}

int store_connection_context_reg3(pxy_agent_t *a)
{
	if(a->isreg == 1 && a->epid != NULL && a->user_ctx_len > 0)
	{
		// epid
		reg3_t* r3 = calloc(sizeof(reg3_t), 1);
		r3->epid = calloc(strlen(a->epid)+1, 1);
		memcpy(r3->epid, a->epid, strlen(a->epid));

		// context
		r3->user_context_len = a->user_ctx_len;
		r3->user_context = calloc(r3->user_context_len, 1);
		memcpy(r3->user_context, a->user_ctx, r3->user_context_len);

		r3->user_id = a->user_id;
		r3->logic_pool_id = a->logic_pool_id;

		r3->remove_time = time(NULL)+REG3_REMOVE_INTERVAL_MIN*60; 
		worker_insert_reg3(r3);

		// send 199 to appbean
		msp_send_req_to_bean(a, 199, 0, NULL);
		return 1;
	}
	else
		return 0;
}

int process_reg3_req(rec_msg_t* msg, pxy_agent_t* a)
{
	UNUSED(msg);
	UNUSED(a);
	return 1;
}

void send_client_flow_to_bean(pxy_agent_t *a)
{
	UNUSED(a);
	//TODO:
	return;

	/*MobileFlowEventArgs args;*/
	/*args.ClientType = a->clienttype;*/
	/*args.DownFlow = a->downflow;*/
	/*args.UpFlow = a->upflow;*/
	/*args.LoginTime.len = strlen(a->logintime);*/
	/*args.LoginTime.buffer = a->logintime;*/
	/*args.LogoffTime.buffer = get_now_time();*/
	/*args.LogoffTime.len = strlen(args.LogoffTime.buffer);*/

	/*rpc_pb_string str_input; */
	/*int inputsz = 1024;  */
	/*str_input.buffer = calloc(inputsz, 1);*/
	/*str_input.len = inputsz;*/
	/*rpc_pb_pattern_pack(rpc_pat_mobilefloweventargs, &args, &str_input);*/

	/*W("105 len %d", str_input.len);*/
	/*msp_send_req_to_bean(a, 105, str_input.len, str_input.buffer);*/
	/*free(str_input.buffer);*/
}

int parse_client_data(pxy_agent_t *agent, rec_msg_t* msg)
{
	buffer_t *b = agent->buffer;
	D("recive len b->len = %lu", b->len);
	if(b->len < 3){
		D("length not enough!");
		return -1;
	}
	int index = 0;
	char_to_int(&msg->len, b, &index, 2);
	D("Parse packet len %d",msg->len);
	if(msg->len > (int)b->len)
		return -1;

	if(packet_len <=0)
		packet_len = msg->len;

	if(packet_len > agent->buf_len) {
		D("Parse packet_len %lu > agent->buf_len %lu", packet_len ,agent->buf_len);
		return -1;
	}

	msg->version = *buffer_read(b, index++);
	char_to_int(&msg->userid, b, &index, 4);
	char_to_int(&msg->cmd, b, &index, 4);
	char_to_int(&msg->seq, b, &index, 2);
	char_to_int(&msg->off, b, &index, 1);
	char_to_int(&msg->format, b, &index, 1);
	char_to_int(&msg->compress, b, &index, 1);
	char_to_int(&msg->client_type, b, &index, 2);
	char_to_int(&msg->client_version, b, &index, 2);
	msg->body_len = msg->len - 1 - msg->off;
	//msg->option_len =  msg->len -21 - msg->body_len;
	msg->option_len =  0;
	if(agent->user_ctx_len == 0){
		msg->user_context_len = 0;
		msg->user_context = NULL;
	}else{
		msg->user_context_len = agent->user_ctx_len;
		msg->user_context = agent->user_ctx;
	}

	if(msg->body_len >0){
		// set body position
		index = msg->off;
		msg->body = calloc(msg->body_len, 1);
		if(msg->body == NULL)
			return -1;

		int t;
		for(t = 0; t < msg->body_len; t++)
			msg->body[t] = *buffer_read(b, index+t);
	}else{	msg->body = NULL; msg->body_len = 0; }

	//index++;// last padding byte
	agent->buf_parsed += packet_len;
	agent->upflow += packet_len;
	packet_len = 0;

	//user is reg state
	if(msg->cmd == 198) {
		if(agent->isreg == 1){
			return -1;
		}
		else {
			int ret = process_reg3_req(msg, agent);
			if(ret < 0) {
				//collection not reg3, close socket
				free(msg->body);
				worker_remove_agent(agent);
				pxy_agent_close(agent);
				return -1;
			}
			else if(ret == 0) {
				// reg3 bad request, return wait client reg3 again
				free(msg->body);
				return -1;
			}
			else {
				// do nothing send reqest to 198 bean
			}
		}
		agent->isreg = 1;
	}

	if(agent->epid != NULL) {
		msg->epid = agent->epidr2; 
		/* 
		if(msg->cmd == 102)
			msg->epid = agent->epidr2; 
		else
			msg->epid = agent->epid; 
 		*/
	}
	else {
		msg->epid = agent->epid = generate_client_epid(msg->client_type, msg->client_version);
		agent->epidr2 = calloc(strlen(agent->epid)+strlen(LISTENERPORT)+2, 1);
		strncat(agent->epidr2, agent->epid, strlen(agent->epid));
		strncat(agent->epidr2, ",", 1);
		strncat(agent->epidr2, LISTENERPORT, strlen(LISTENERPORT));
		W("REG2 epid %s", agent->epidr2);
		agent->clienttype = msg->client_type;
		worker_insert_agent(agent);
	}
	msg->epid = agent->epidr2; 
	D("msp->epid %s", msg->epid);
	msg->logic_pool_id = agent->logic_pool_id;
	return 1;
}


void release_rpc_message(rec_msg_t* msg)
{
	if(msg->body_len > 0 && msg->body != NULL)
		free(msg->body);
}

/*void rpc_response(rpc_connection_t *c, rpc_int_t code, void *output, size_t output_len,void *input, size_t input_len, void *data)*/
/*{*/
	/*UNUSED(c);*/
	/*UNUSED(input);*/
	/*UNUSED(input_len);*/
	/*rpc_async_req_t* rt = (rpc_async_req_t*)data;*/

	/*W("cmd %d code %d", rt->req->cmd, (int)code);*/
	/*if (code == RPC_CODE_OK) {*/
		/*McpAppBeanProto rsp;*/
		/*rpc_pb_string str = {output, output_len};*/
		/*int r = rpc_pb_pattern_unpack(rpc_pat_mcpappbeanproto, &str, &rsp);*/
		/*if (r < 0) {*/
			/*D("rpc response unpack fail");*/
			/*// send 500 to client*/
			/*send_response_client(rt->req, rt->a, 504);*/
			/*free(rt->rpc_bf);*/
			/*free(data);*/
			/*return;*/
		/*}*/

		/*// 109 msp send reg3, 105 msp send flow, 103 & unreg reg3 overtime msp send unreg;*/
		/*if(rsp.Cmd == 199 || rsp.Cmd == 105 || (rsp.Cmd == 103 && rt->msp_unreg == 1)) {*/
			/*D("socket closed msp send unreg!");*/
			/*free(rt->rpc_bf);*/
			/*free(data);*/
			/*return;*/
		/*}*/

		/*rec_msg_t rp_msg;*/
		/*rp_msg.cmd = rsp.Cmd;*/
		/*rp_msg.body_len = rsp.Content.len;*/
		/*rp_msg.body = rsp.Content.buffer;*/
		/*rp_msg.userid = rsp.UserId;*/
		/*rp_msg.seq = rsp.Sequence;*/
		/*rp_msg.format = rsp.Opt;*/
		/*rp_msg.compress = rsp.ZipFlag;*/
		/*char* func = get_cmd_func_name(rsp.Cmd);*/
		/*D("rpc %s func response, seq is %d", func, rp_msg.format);*/
		/*UNUSED(func);*/
		/*// reg2 response*/
		/*if(rp_msg.cmd == 102 && rt->a->user_ctx == NULL) {*/
			/*rt->a->isreg = 1;*/
			/*rt->a->user_ctx_len = rsp.UserContext.len;*/
			/*rt->a->user_ctx = calloc(rsp.UserContext.len, 1);*/
			/*memcpy(rt->a->user_ctx, rsp.UserContext.buffer, rsp.UserContext.len);*/
			/*UserContext us_ctx;*/
			/*rpc_pb_string str_uc = {rsp.UserContext.buffer, rsp.UserContext.len};*/
			/*int t = rpc_pb_pattern_unpack(rpc_pat_usercontext, &str_uc, &us_ctx);*/

			/*if(t<0)*/
			/*{*/
				/*D("unpack user context fail");*/
			/*}*/
			/*else {*/
				/*rt->a->user_id = us_ctx.UserId;*/
				/*rt->a->logic_pool_id = us_ctx.LogicalPool;*/
				/*rt->a->logintime = get_now_time();*/
			/*}*/
		/*}*/

		/*int len = 0;*/
		/*char* d = NULL;*/
		/*d = get_send_data(&rp_msg, &len);*/
		/*D("fd %d, d %p, len %d", rt->a->fd, d, len);*/
		/*send_to_client(rt->a, d, (size_t)len);*/
		/*free(d);*/
		/*if(rp_msg.cmd == 103) {*/
			/*rt->a->isreg = 0;*/
			/*worker_remove_agent(rt->a);*/
			/*pxy_agent_close(rt->a);*/
		/*}*/
	/*}*/
	/*else {*/
		/*if(rt->req == NULL || rt->req->cmd == 199 || rt->req->cmd == 105 ||( rt->req->cmd == 103 && rt->msp_unreg == 1))*/
		/*{*/
			/*D("socket closed msp send unreg!");*/
			/*free(rt->rpc_bf);*/
			/*free(data);*/
			/*return;*/
		/*}*/

		/*// send 500 server error*/
		/*send_response_client(rt->req, rt->a, 500);*/
		/*D("send rpc response fail");*/
		/*if(rt->req->cmd == 103) {*/
			/*worker_remove_agent(rt->a);*/
			/*pxy_agent_close(rt->a);*/
		/*}*/
	/*}*/
	/*free(rt->rpc_bf);*/
	/*free(data);*/
/*}*/

/*int send_rpc_server(rec_msg_t* req, char* proxy_uri, pxy_agent_t *a, int msp_unreg)*/
/*{*/
	/*W("cmd %d prxy uri %s", req->cmd, proxy_uri);*/
	/*rpc_proxy_t *p ;*/

	/*upstream_t *us = us_get_upstream(upstream_root, proxy_uri);*/
	/*if(!us) {*/
		/*W("no proxy,malloc one");*/
		/*us = pxy_calloc(sizeof(*us));*/
		/*if(!us) {*/
			/*E("cannot malloc for upstream_t");*/
			/*return -1;*/
		/*}*/

		/*rpc_client_t *cnt = rpc_client_new();*/
		/*p = rpc_client_connect(cnt, proxy_uri);*/
		/*p->setting.connect_timeout = 500;*/
		/*p->setting.request_timeout = 100;//TODO: this value should be more exactly*/

		/*us->uri = strdup(proxy_uri);*/
		/*us->proxy = p;*/
		/*us_add_upstream(upstream_root, us);*/
	/*}*/
	/*p = us->proxy;*/

	/*rpc_args_init();*/
	/*McpAppBeanProto args; */
	/*get_rpc_arg(&args, req);*/

	/*rpc_pb_string str_input; */
	/*int inputsz = 1024;  */
	/*str_input.buffer = calloc(inputsz, 1);*/
	/*str_input.len = inputsz;*/
	/*rpc_pb_pattern_pack(rpc_pat_mcpappbeanproto, &args, &str_input);*/
	/*char* func_name = get_cmd_func_name(req->cmd);*/

	/*D("rpc call func name %s", func_name);*/
	/*rpc_async_req_t* rt = calloc(sizeof(rpc_async_req_t), 1);*/
	/*rt->a = a;*/
	/*rt->req = req;*/
	/*rt->rpc_bf = str_input.buffer;*/
	/*rt->msp_unreg = msp_unreg;*/
	/*rpc_int_t code = rpc_call(p, "MCP", func_name, str_input.buffer, str_input.len, NULL, NULL);*/

	/*//we only handle error here */
	/*if(code != RPC_CODE_OK) {*/
		/*//TODO: send the error response to the client */
	/*}*/
	/*D("code is %d", (int) RPC_CODE_OK);*/

	/*free(str_input.buffer);*/
	/*free(args.CompactUri.buffer);*/
	/*return 0;*/
/*}*/

void send_response_client(rec_msg_t* req,  pxy_agent_t* a, int code) 
{
	rec_msg_t rp_msg;
	rp_msg.cmd = req->cmd;
	rp_msg.body_len = 3;
	if(code == 504)
		rp_msg.body = OVERTIME;
	else if(code == 400)
		rp_msg.body = REQERROR;
	else if(code == 411)
		rp_msg.body = NOTEXIST;
	else 
		rp_msg.body = SERVERERROR;

	rp_msg.userid = req->userid;
	rp_msg.seq = req->seq;
	rp_msg.format = 128;
	rp_msg.compress = 0; 
	rp_msg.client_type = req->client_type;
	rp_msg.client_version = req->client_version;
	int len;
	char* d = get_send_data(&rp_msg, &len);
	D("fd %d, d %p, len %d", a->fd, d, len);
	send_to_client(a, d, (size_t)len);
	free(d);
}
 
char __url[32] = "tcp://10.10.10.81:7777";

static int agent_to_beans(pxy_agent_t *a, rec_msg_t* msg, int msp_unreg)
{
	UNUSED(a);
	char *url = NULL;
	get_app_url(msg->cmd, 1, NULL, NULL, NULL, &url);
	if(url == NULL) {
		D("cmd %d url is null", msg->cmd);
		return -1;
	}
	else {
		D("cmd %d url=%s", msg->cmd, url);
	}

	msg->uri = url;
	if(mrpc_us_send(msg) < 0) {
		return -1;
	}

	free(url);
	return 0;
}


static int process_client_req(pxy_agent_t *agent)
{
	rec_msg_t msg;
	if(parse_client_data(agent, &msg) < 0)
	{	
		release_rpc_message(&msg);
		return -1;
	}

	if(agent_to_beans(agent,&msg,0) < 0) {
		release_rpc_message(&msg);
		return -1;
	}
	release_rpc_message(&msg);
	pxy_agent_buffer_recycle(agent);
	return 0;
}

int pxy_agent_buffer_recycle(pxy_agent_t *agent)
{
	int rn      = 0;
	size_t n    = agent->buf_parsed;
	buffer_t *b = agent->buffer,*t;

	if(b){
		D("before recycle:the agent->offset:%zu,agent->len:%zu,agent->parsed:%zu",
				agent->buf_offset,
				agent->buf_len,
				agent->buf_parsed);
	}

	while(b!=NULL && n >= b->len) {
		n  -= b->len;
		rn += b->len;

		t = b->next;
		buffer_release(b,worker->buf_pool,worker->buf_data_pool);
		b = t;

		D("b:%p,n:%zu",b,n);
	}

	agent->buffer     = b;
	agent->buf_len   -= rn;
	agent->buf_parsed -= rn;

	D("after recycle:the agent->offset:%zu,agent->len:%zu,agent->parsed:%zu",
			agent->buf_offset,
			agent->buf_len,
			agent->buf_parsed);

	return rn;
}

buffer_t* agent_get_buf_for_read(pxy_agent_t *agent)
{
	buffer_t *b = buffer_fetch(worker->buf_pool,worker->buf_data_pool);

	if(b == NULL) {
		D("no buf available"); 
		return NULL;
	}

	if(agent->buffer == NULL) {
		agent->buffer = b;
	}
	else {
		buffer_append(b,agent->buffer);
	}

	return b;
}

void pxy_agent_close(pxy_agent_t *a)
{
	// send flow to bean 105
	send_client_flow_to_bean(a);
	free(a->epid);
	a->epid = NULL;
	free(a->epidr2);
	a->epidr2 = NULL;
	a->user_ctx_len = 0;
	free(a->user_ctx);
	a->user_ctx = NULL;
	free(a->logintime);
	a->logintime = NULL;
	free(a->logouttime);
	a->logouttime = NULL;
	buffer_t *b;

	if(a->fd > 0){
		/* close(agent->fd); */
		if(ev_del_file_item(worker->ev,a->fd) < 0){
			D("del file item err, errno is %d",errno);
		}

		D("close the socket");
		close(a->fd);
	}

	while(a->buffer){
		b = a->buffer;
		a->buffer = b->next;

		if(b->data){
			mp_free(worker->buf_data_pool,b->data);
		}
		mp_free(worker->buf_pool,b);
	}

	mp_free(worker->agent_pool,a);
}


pxy_agent_t * pxy_agent_new(mp_pool_t *pool,int fd,int userid)
{
	pxy_agent_t *agent = mp_alloc(pool);
	if(!agent){
		D("no mempry for agent"); 
		goto failed;
	}

	agent->fd         = fd;
	agent->user_id    = userid;
	agent->buf_sent   = 0;
	agent->buf_parsed = 0;
	agent->buf_offset = 0;
	agent->buf_list_n = 0;
	agent->buf_len = 0;
	agent->bn_seq = 10000;
	agent->epid = NULL;
	agent->user_ctx = NULL;
	agent->buffer = NULL;
	agent->isreg = 0;
	agent->upflow = 0;
	agent->downflow = 0;
	agent->send_buf = NULL;
	agent->send_buf_len = 0;
	agent->send_buf_offset = 0;
	return agent;

failed:
	if(agent){
		mp_free(worker->agent_pool,agent);
	}
	return NULL;
}

// client data
void agent_recv_client(ev_t *ev,ev_file_item_t *fi)
{
	UNUSED(ev);

	int n;
	buffer_t *b;
	pxy_agent_t *agent = fi->data;

	D("inter agent_recv_client"); 
	if(!agent)
	{
		W("fd has no agent,ev->data is NULL,close the fd");
		close(fi->fd); return;
	}

	while(1) {
		b = agent_get_buf_for_read(agent);
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
			D("break from receive loop");	
			break;
		}
	}
	D("buffer-len %zu", agent->buf_len);
	if(process_client_req(agent) < 0) {
		W("echo read test fail!");
		goto failed;
	}
	return;

failed:
	W("failed, prepare close!");
	//Add to reg3 rbtree
	store_connection_context_reg3(agent);
	worker_remove_agent(agent);
	pxy_agent_close(agent);
	free(fi);
	return;
}
