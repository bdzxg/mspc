#include "proxy.h"
#include "agent.h"
#include "include/rpc_client.h"
#include "include/zookeeper.h"
#include <pthread.h>
#include "route.h"
#include "include/rpc_args.h"
#include "ClientHelper.c"

extern pxy_worker_t *worker;
size_t packet_len;
static char* PROTOCOL = "MCP/3.0";

int char_to_int(buffer_t* buf, int* off, int len)
{
	int i;
	char tmp[4] = {0,0,0,0};
	for(i = 0 ; i < len; i++)
	{
		tmp[i] = *buffer_read(buf, *off);
		(*off)++;
	}
	return *(int*)tmp;
}

int parse_client_data(pxy_agent_t *agent, rec_msg_t* msg)
{
	buffer_t *b = agent->buffer;
	D("recive len b->len = %lu", b->len);
	if(agent->buf_len - agent->buf_parsed< 3){
		D("length not enough!");
		return 0;
	}
	int index = 1;
	msg->len = char_to_int(b, &index, 2);
	D("Parse packet len %d",msg->len);
	if(packet_len <=0)
		packet_len = msg->len;

	if(packet_len > agent->buf_len)
	{
		D("Parse packet_len %lu > agent->buf_len %lu", packet_len ,agent->buf_len);
		return 0;
	}

	msg->version = *buffer_read(b, index++);
	msg->userid = char_to_int(b, &index, 4);
	msg->cmd = char_to_int(b, &index, 2);
	msg->seq = char_to_int(b, &index, 2);
	msg->off = char_to_int(b, &index, 1);
	msg->format = char_to_int(b, &index, 1);
	msg->compress = char_to_int(b, &index, 1);
	msg->client_type = char_to_int(b, &index, 2);
	msg->client_version = char_to_int(b, &index, 2);
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
		msg->epid = agent->epid; 
	else
	{
		msg->epid = agent->epid = generate_client_epid(msg->client_type, msg->client_version);
		map_insert(&worker->root, agent);
	}
	msg->logic_pool_id = agent->user_context.LogicalPool;
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

void rpc_response(rpc_connection_t *c, rpc_int_t code, void *output, size_t output_len,void *input, size_t input_len, void *data)
{
		if(code != RPC_CODE_OK || output_len == 0){
				D("rpc return code %d, buf_out_size %zu", (int)code, output_len); rpc_free(input);
				return;
		}

		McpAppBeanProto rsp;
		rpc_pb_string str = {output, output_len};
		int r = rpc_pb_pattern_unpack(rpc_pat_mcpappbeanproto, &str, &rsp);
		if (r < 0) {
				D("rpc response unpack fail");
				rpc_free(input);
				return;
		}
		
		pxy_agent_t* a = (pxy_agent_t*)data;
	    char* func = get_cmd_func_name(rsp.Cmd);
		if(rsp.Cmd == 103 && a->msp_unreg == 1)
		{
				D("socket close msp send unreg!");
				rpc_free(input);
				return;
		}

		rec_msg_t msg;
		msg.cmd = rsp.Cmd;
		msg.body_len = rsp.Content.len;
		msg.body = rsp.Content.buffer;
		msg.userid = rsp.UserId;
		msg.seq = rsp.Sequence;
		msg.format = rsp.Opt;
		msg.compress = rsp.ZipFlag;
		
		D("rpc %s func response, seq is %d", func, msg.format);
		//client needn't usercontext
		// client needn't epid
		if(msg.cmd == 102 && a->user_ctx == NULL)
		{
			a->user_ctx_len = rsp.UserContext.len;
			a->user_ctx = calloc(rsp.UserContext.len, 1);
			memcpy(a->user_ctx, rsp.UserContext.buffer, rsp.UserContext.len);
		
			UserContext us_ctx;
			rpc_pb_string str_uc = {rsp.UserContext.buffer, rsp.UserContext.len};
			int t = rpc_pb_pattern_unpack(rpc_pat_usercontext, &str_uc, &us_ctx);
			if(t<0)
				D("unpack user context fail");
			else {
				a->user_id = a->user_context.UserId = us_ctx.UserId;
				a->user_context.LogicalPool = us_ctx.LogicalPool;
			}
		}
		int len;
		char* d = get_send_data(&msg, &len);
		D("a %p", a);
		D("fd %d, d %p, len %d", a->fd, d, len);
		send(a->fd, d, len, 0);
		if(msg.cmd == 103)
		{
			pxy_agent_close(a);
			map_remove(&worker->root, a->epid);
		}
		rpc_free(input);
}

void release_rpc_message(rec_msg_t* msg)
{
	//if(msg->option != NULL)
	//	free(msg->option);
	if(msg->body_len > 0 && msg->body != NULL)
		free(msg->body);
}

int send_rpc_server(rec_msg_t* msg, char* proxy_uri, pxy_agent_t *a)
{
    rpc_client_t *c= rpc_client_new();
	rpc_proxy_t *p = rpc_client_connect(c, proxy_uri);
	if(p == NULL){
		D("creat rpc client is NULL");
		return -1;
	}
	rpc_args_init();
	McpAppBeanProto args; 
	
	get_rpc_arg(&args, msg);

	rpc_pb_string str_input; 
	int inputsz = 1024;  
	str_input.buffer = calloc(inputsz, 1);
	str_input.len = inputsz;
	rpc_pb_pattern_pack(rpc_pat_mcpappbeanproto, &args, &str_input);
	char* func_name = get_cmd_func_name(msg->cmd);
	D("rpc call func name %s", func_name);
	rpc_call_async(p, "MCP", func_name, str_input.buffer, str_input.len, rpc_response, a); 
	free(args.CompactUri.buffer);
	return 0;
}

void agent_send_client(rec_msg_t* rec_msg, pxy_agent_t *agent)
{
	// send to client
	int len = 0;
    char* data = get_send_data(rec_msg, &len);
	D("send to client fd %d", agent->fd);
	send(agent->fd, data, len, 0);
}

int agent_to_server(pxy_agent_t *a, rec_msg_t* msg)
{
	char *url;
    get_app_url(msg->cmd, 1, NULL, NULL, NULL, &url);
    D("cmd %d url=%s",msg->cmd, url);
    //get proxy uri
	if(send_rpc_server(msg, url, a) < 0)
	{
        free(url);
	   	return -1;
	}
	free(url);
	return 0;
}

void msp_send_unreg(pxy_agent_t* a)
{
	if(a->epid != NULL)
	{
		rec_msg_t msg;
		msg.cmd = 103;
		msg.userid = a->user_id;
		msg.logic_pool_id = a->user_context.LogicalPool;
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

int agent_echo_read_test(pxy_agent_t *agent)
{
    rec_msg_t msg;
	//msg = calloc(sizeof(rec_msg_t), 1);
	parse_client_data(agent, &msg);
	if(agent_to_server(agent,&msg) < 0)
	{
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
	if(a->epid != NULL)
		D("pxy_agent_close release epid %s and user context", a->epid);
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
			D("no buf for read");
			msp_send_unreg(agent);
			pxy_agent_close(agent);
			map_remove(&worker->root, agent->epid);
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
				D("read error,errno is %d",errno);
				goto failed;
			}
		}

		if(n == 0)
		{
			D("socket fd #%d closed by peer",fi->fd);
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
	if(agent_echo_read_test(agent) < 0)
	{
		D("fail!");
	    goto failed;
	}
	return;

failed:
	D("failed!");
	msp_send_unreg(agent);
	map_remove(&worker->root, agent->epid);
	pxy_agent_close(agent);
	return;
}
