#include "proxy.h"
#include "agent.h"
#include "include/rpc_client.h"
#include "include/zookeeper.h"
#include <pthread.h>
#include "route.h"
#include "include/rpc_args.h"
#include "ClientHelper.c"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;

size_t packet_len;
static char* PROTOCOL = "MCP/3.0";
// 504
static char OVERTIME[3] = {8, 248, 3};
// 500
static char SERVERERROR[3] = {8, 244, 3};

void char_to_int(int*v, buffer_t* buf, int* off, int len)
{
	*v = 0;
	char* t = (char*)v;
	int i;
	for(i = 0 ; i < len; i++)
	{
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
	if(agent->epid != NULL)
	{
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

void release_reg3(reg3_t* r3)
{
	free(r3->epid);
	free(r3->user_context);
	free(r3);
}

int store_connection_context_reg3(pxy_agent_t *a)
{
		if(a->epid != NULL && a->user_ctx_len > 0)
		{
				reg3_t* r3 = calloc(sizeof(reg3_t), 1);
				r3->epid = calloc(strlen(a->epid)+1, 1);
				memcpy(r3->epid, a->epid, strlen(a->epid));

				r3->user_context_len = a->user_ctx_len;
				r3->user_context = calloc(r3->user_context_len, 1);
				memcpy(r3->user_context, a->user_ctx, r3->user_context_len);
				
				r3->user_id = a->user_id;
				r3->logic_pool_id = a->logic_pool_id;

				worker_insert_reg3(r3);
				return 1;
		}
		else
				return 0;
}

int parse_client_data(pxy_agent_t *agent, rec_msg_t* msg)
{
	buffer_t *b = agent->buffer;
	D("recive len b->len = %lu", b->len);
	if(b->len < 3){
		D("length not enough!");
		return -1;
	}
	int index = 1;
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
	char_to_int(&msg->cmd, b, &index, 2);
	char_to_int(&msg->seq, b, &index, 2);
	char_to_int(&msg->off, b, &index, 1);
	char_to_int(&msg->format, b, &index, 1);
	char_to_int(&msg->compress, b, &index, 1);
	char_to_int(&msg->client_type, b, &index, 2);
	char_to_int(&msg->client_version, b, &index, 2);
	msg->body_len = msg->len - 1 - msg->off;
	//msg->option_len =  msg->len -21 - msg->body_len;
	if(agent->user_ctx_len == 0){
		msg->user_context_len = 0;
		msg->user_context = NULL;
	}else{
		msg->user_context_len = agent->user_ctx_len;
		msg->user_context = agent->user_ctx;
	}
	/* 
	   if(msg->option_len >0){
	   //skip unused padding 0
	   index++;
	   msg->option = calloc(msg->option_len, 1);
	   if(msg->option == NULL)
	   return -1;
	
	   int t;
	   for(t = 0; t < msg->option_len; t++)
	   msg->option[t] = *buffer_read(b,index+t);
	   }else{
	   msg->option_len = 0;
	   msg->option = NULL;
	   }
	*/
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
	packet_len = 0;
    
	if(agent->epid != NULL)
	{
		if(msg->cmd == 102)
			msg->epid = agent->epidr2; 
		else
			msg->epid = agent->epid; 
	}
	else {
		msg->epid = agent->epid = generate_client_epid(msg->client_type, msg->client_version);
		agent->epidr2 = calloc(strlen(agent->epid)+strlen(LISTENERPORT)+2, 1);
		strncat(agent->epidr2, agent->epid, strlen(agent->epid));
		strncat(agent->epidr2, ",", 1);
		strncat(agent->epidr2, LISTENERPORT, strlen(LISTENERPORT));

		worker_insert_agent(agent);
	}
	msg->logic_pool_id = agent->logic_pool_id;
	return 1;
}

char* Get_CompactUri(rec_msg_t* msg)
{
	char* ret;
	ret = malloc(36);
	if(ret == NULL)
		return ret;
 	sprintf(ret, "id:%d;p=%d;", msg->userid,msg->logic_pool_id);	
	return ret;
}
 
void get_rpc_arg(McpAppBeanProto* args, rec_msg_t* msg) 
{
	args->Protocol.len = strlen(PROTOCOL);
	args->Protocol.buffer = PROTOCOL;
 	args->Cmd = msg->cmd;
	char* uri = Get_CompactUri(msg);
	args->CompactUri.len = strlen(uri);
	args->CompactUri.buffer = uri;
	args->UserId = msg->userid;
	args->Sequence = msg->seq;
	args->Opt = msg->format;
	
	if(msg->user_context_len > 0){
		args->UserContext.len = msg->user_context_len;
		args->UserContext.buffer = msg->user_context;
	}else{ args->UserContext.len = 0;  args->UserContext.buffer = NULL;}

	D("request UserContext.len %d", args->UserContext.len);
	if(msg->body_len > 0){	
		args->Content.len = msg->body_len;
		args->Content.buffer = msg->body;
	}else{ args->Content.len = 0; args->Content.buffer = NULL;}
	
	args->Epid.len = strlen(msg->epid);
	args->Epid.buffer = msg->epid;
	args->ZipFlag = msg->compress;
}	

void release_rpc_message(rec_msg_t* msg)
{
	//if(msg->option != NULL)
	//	free(msg->option);
	if(msg->body_len > 0 && msg->body != NULL)
		free(msg->body);
}

int send_rpc_server(rec_msg_t* req, char* proxy_uri, pxy_agent_t *a)
{
	W("prxy uri %s", proxy_uri);
	rpc_proxy_t *p ;
	upstream_t *us = us_get_upstream(upstream_root, proxy_uri);
	if(!us) {
		W("no proxy,malloc one");
		us = pxy_calloc(sizeof(*us));
		if(!us) {
			E("cannot malloc for upstream_t");
			return -1;
		}

		// sync rpc method
		p = rpc_proxy_new(proxy_uri);
		if(p == NULL) {
			D("fuck get proxy_uri fail");
			return -1;
		}	
		p->setting.connect_timeout = 500;
		p->setting.request_timeout = 6000;
		
		us->uri = strdup(proxy_uri);
		us->proxy = p;
		us_add_upstream(upstream_root, us);
	}
	p = us->proxy;

	//TODO: check if proxy is available both in tcp and ZK

	void *out;
	size_t outsize;

	rpc_args_init();
	McpAppBeanProto args; 
	get_rpc_arg(&args, req);

	rpc_pb_string str_input; 
	int inputsz = 1024;  
	str_input.buffer = calloc(inputsz, 1);
	str_input.len = inputsz;
	rpc_pb_pattern_pack(rpc_pat_mcpappbeanproto, &args, &str_input);
	char* func_name = get_cmd_func_name(req->cmd);
	D("rpc call func name %s", func_name);
	int code = rpc_call(p,"MCP", func_name, str_input.buffer, str_input.len, &out, &outsize);

	if (code == RPC_CODE_OK) {
		McpAppBeanProto rsp;
		rpc_pb_string str = {out, outsize};
		int r = rpc_pb_pattern_unpack(rpc_pat_mcpappbeanproto, &str, &rsp);
		if (r < 0) {
			D("rpc response unpack fail");
			// send 500 to client
			send_response_client(req, a, 504);
			free(str_input.buffer);
			free(args.CompactUri.buffer);
			return -1;
		}

		if(rsp.Cmd == 103 && a->msp_unreg == 1) {
			free(str_input.buffer);
			free(args.CompactUri.buffer);
			D("socket closed msp send unreg!");
			return 0;
		}

		
		char* func = get_cmd_func_name(rsp.Cmd);
		rec_msg_t rp_msg;
		rp_msg.cmd = rsp.Cmd;
		rp_msg.body_len = rsp.Content.len;
		rp_msg.body = rsp.Content.buffer;
		rp_msg.userid = rsp.UserId;
		rp_msg.seq = rsp.Sequence;
		rp_msg.format = rsp.Opt;
		rp_msg.compress = rsp.ZipFlag;
		D("rpc %s func response, seq is %d", func, rp_msg.format);
		if(rp_msg.cmd == 102 && a->user_ctx == NULL) {
			a->user_ctx_len = rsp.UserContext.len;
			a->user_ctx = calloc(rsp.UserContext.len, 1);
			memcpy(a->user_ctx, rsp.UserContext.buffer, rsp.UserContext.len);
			UserContext us_ctx;
			rpc_pb_string str_uc = {rsp.UserContext.buffer, rsp.UserContext.len};
			int t = rpc_pb_pattern_unpack(rpc_pat_usercontext, &str_uc, &us_ctx);
			if(req->epid != NULL)
				D("REG2 response epid is %s", req->epid);
			if(t<0)
				{
					D("unpack user context fail");
				}
			else {
				a->user_id = us_ctx.UserId;
				a->logic_pool_id = us_ctx.LogicalPool;
			}
		}
		int len;
		char* d = get_send_data(&rp_msg, &len);
		D("fd %d, d %p, len %d", a->fd, d, len);
		int n  = send(a->fd, d, len, 0);
		if( n < len ) {
			W("send %d:%d", n , len);
		}
		free(d);
		if(rp_msg.cmd == 103) {
			worker_remove_agent(a);
			pxy_agent_close(a);
		}
		free(str_input.buffer);
	}
	else {
		D("code is %d", code);
		free(str_input.buffer);
		if(req->cmd == 103 && a->msp_unreg == 1) {
			free(args.CompactUri.buffer);
			D("socket closed msp send unreg!");
			return 0;
		}

		// send 504 overtime to client
		send_response_client(req, a, 504);
		D("send rpc response fail");
		if(req->cmd == 103) {
			worker_remove_agent(a);
			pxy_agent_close(a);
		}
	}

	free(args.CompactUri.buffer);
	return 0;
}

void send_response_client(rec_msg_t* req,  pxy_agent_t* a, int code) 
{
	rec_msg_t rp_msg;
	rp_msg.cmd = req->cmd;
	rp_msg.body_len = 3;
	if(code == 504)
		rp_msg.body = OVERTIME;
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
	int n = send(a->fd, d, len, 0);
	if( n < len) {
		W("send %d-%d", len , n);
	}
	free(d);
}

int agent_to_server(pxy_agent_t *a, rec_msg_t* msg)
{
	char *url;
	get_app_url(msg->cmd, 1, NULL, NULL, NULL, &url);
	D("cmd %d url=%s", msg->cmd, url);
	//get proxy uri
	if(send_rpc_server(msg, url, a) < 0) {
		free(url);
		return -1;
	}
	free(url);
	return 0;
}

void msp_send_unreg(pxy_agent_t* a)
{
	if(a->user_ctx != NULL)
		{
			rec_msg_t msg;
			msg.cmd = 103;
			msg.userid = a->user_id;
			msg.logic_pool_id = a->logic_pool_id;
			msg.seq = 100000;
			msg.format = 0;
			msg.user_context_len = a->user_ctx_len;
			msg.user_context = a->user_ctx;
			msg.body_len = 0;
			msg.epid = a->epid;
			msg.compress = 0;
			a->msp_unreg = 1;
			agent_to_server(a, &msg);
		}
}

int process_client_req(pxy_agent_t *agent)
{
	rec_msg_t msg;
	//msg = calloc(sizeof(rec_msg_t), 1);
	if(parse_client_data(agent, &msg) < 0)
	{	
		release_rpc_message(&msg);
		return -1;
	}

	if(agent_to_server(agent,&msg) < 0) {
		release_rpc_message(&msg);
		return -1;
	}
	release_rpc_message(&msg);
	pxy_agent_buffer_recycle(agent);
	return 0;
}

int 
pxy_agent_buffer_recycle(pxy_agent_t *agent)
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

void 
pxy_agent_close(pxy_agent_t *a)
{
	//if(a->epid != NULL)
	//	D("pxy_agent_close release epid %s and user context", a->epid);
	free(a->epid);
	a->epid = NULL;
	a->user_ctx_len = 0;
	free(a->user_ctx);
	a->user_ctx = NULL;
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

buffer_t* 
agent_get_buf_for_read(pxy_agent_t *agent)
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
	agent->msp_unreg = 0;
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

	while(1) 
		{
			b = agent_get_buf_for_read(agent);
			if(b == NULL)
				{
					E("no buf for read");
					goto failed;
				}

			n = recv(fi->fd,b->data,BUFFER_SIZE,0);
			D("recv %d bytes",n);

			if(n < 0)
				{
					if(errno == EAGAIN || errno == EWOULDBLOCK)
						{
							break;
						}
					else
						{
							W("read error,errno is %d",errno);
							goto failed;
						}
				}

			if(n == 0)
				{
					W("socket fd #%d closed by peer",fi->fd);
					goto failed;
				}

			b->len = n;
			agent->buf_len += n;

			if(n < BUFFER_SIZE)
				{	
					D("break from receive loop");	
					break;
				}
		}
	D("buffer-len %zu", agent->buf_len);
	if(process_client_req(agent) < 0)
	{
			W("echo read test fail!");
			goto failed;
	}
	return;

 failed:
	W("failed, prepare close!");
	//msp_send_unreg(agent);
	// todo Add to reg3 rbtree
	store_connection_context_reg3(agent);
	worker_remove_agent(agent);
	pxy_agent_close(agent);
	free(fi);
	return;
}
