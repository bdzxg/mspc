#include "proxy.h"
#include "agent.h"
#include "include/rpc_client.h"
#include "include/zookeeper.h"
#include <pthread.h>
//#include "route.h"
#include "include/rpc_args.h"
#include "ClientHelper.c"

extern pxy_worker_t *worker;
size_t packet_len;
static char* PROTOCOL = "MCP/3.0";
char* agent_epid = NULL;
char* agent_usercontext = NULL;

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
	msg->option_len =  msg->len -21 - msg->body_len;
	if(agent_usercontext == NULL){
		msg->user_context_len = 0;
		msg->user_context = NULL;
	}else{
		msg->user_context_len = strlen(agent_usercontext);
		msg->user_context = agent_usercontext;
	}
/* 	
	D("Parse version %d", msg->version);
	D("Parse userid %d", msg->userid);
	D("Parse cmd %d", msg->cmd);	
	D("Parse seq %d", msg->seq);
	D("Parse off %d", msg->off);
	D("Parse fromat %d", msg->format);
	D("Parse compress %d", msg->compress);
	D("Parse client_type %d", msg->client_type);
	D("Parse client_version %d", msg->client_version);
	D("Option length is %d", msg->option_len);
	D("Body length is %d", msg->body_len);
*/
	if(msg->option_len >0)
	{
		//skip unused padding 0
		index++;
		msg->option = calloc(msg->option_len, 1);
		if(msg->option == NULL)
			return -1;
	
		int t;
		for(t = 0; t < msg->option_len; t++)
		{
			msg->option[t] = *buffer_read(b,index+t);
		//	D("msg->option[%d] is %d", t,msg->option[t]); 
		}
	}else{
		msg->option= NULL;
		//D("body is NULL");
	}

	if(msg->body_len >0)
	{
		// set body position
		index = msg->off;
		msg->body = calloc(msg->body_len, 1);
		if(msg->body == NULL)
			return -1;
		
		int t;
		for(t = 0; t < msg->body_len; t++)
		{
			msg->body[t] = *buffer_read(b, index+t);
			//D("body[%d] is %d", t,msg->body[t]); 
		}
	}else{
		msg->body = NULL;
		//D("body is NULL");
	}
	//index++;// last padding byte
	agent->buf_parsed += packet_len;
	packet_len = 0;
    
	if(agent_epid != NULL)
		msg->epid = agent_epid; 
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
	
	if(msg->user_context != NULL){
		args->UserContext.len = strlen(msg->user_context);
		args->UserContext.buffer = msg->user_context;
	}else{ args->UserContext.len = 0;  args->UserContext.buffer = NULL;}

	if(msg->body_len > 0){	
		args->Content.len = msg->body_len;
		args->Content.buffer = msg->body;
	}else{ args->Content.len = 0; args->Content.buffer = NULL;}
	
	if(msg->epid == NULL)
		msg->epid = agent_epid = generate_client_epid(msg->client_type, msg->client_version);
	
	args->Epid.len = strlen(msg->epid);
	args->Epid.buffer = msg->epid;
	args->ZipFlag = msg->compress;
/*	D("args->protocol %s", args->Protocol.buffer); D("args->cmd %u", args->Cmd);
	D("args->compacturi %s", args->CompactUri.buffer); 	D("args->userid %d", args->UserId);
	D("args->sequence %d", args->Sequence); D("args->opt %d", args->Opt);
	D("args->usercontext.len %zu",args->UserContext.len); D("args->content.len %zu", args->Content.len);
	D("args->content.data %s", args->Content.buffer); D("args->epid %s", args->Epid.buffer);
	D("args->zipflag %d", args->ZipFlag);*/
}	

void rpc_response(rpc_connection_t *c, rpc_int_t code, void *output, size_t output_len,void *input, size_t input_len, void *data)
{
		if(code != RPC_CODE_OK || output_len == 0){
				D("rpc return code %d,input len %zu, buf_out_size %zu", (int)code, input_len, output_len);
				return;
		}

		McpAppBeanProto rsp;
		rpc_pb_string str = {output, output_len};
		int r = rpc_pb_pattern_unpack(rpc_pat_mcpappbeanproto, &str, &rsp);
		if (r < 0) {
				D("rpc response unpack fail");
				return;
		}

		rec_msg_t msg;
		msg.cmd = rsp.Cmd;
		msg.body_len = rsp.Content.len;
		msg.body = rsp.Content.buffer;
		msg.userid = rsp.UserId;
		msg.seq = rsp.Sequence;
		msg.format = rsp.Opt;
		msg.user_context = rsp.UserContext.buffer;
	    msg.user_context_len = rsp.UserContext.len;	
		msg.compress = rsp.ZipFlag;
		msg.epid = calloc(rsp.Epid.len+1, 1); 
		memcpy(msg.epid, rsp.Epid.buffer, rsp.Epid.len);
	
		pxy_agent_t* a = (pxy_agent_t*)data;
		int len;
		char* d = get_send_data(&msg, &len);
		send(a->fd, d, len, 0);
		free(msg.epid);
		rpc_free(input);
}

int send_rpc_server(rec_msg_t* msg, char* proxy_uri, pxy_agent_t *agent)
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

	D("before pack");
	rpc_pb_string str_input; 
	int inputsz = 1024;  
	str_input.buffer = calloc(inputsz, 1);
	str_input.len = inputsz;
	rpc_pb_pattern_pack(rpc_pat_mcpappbeanproto, &args, &str_input);
	char* func_name = get_cmd_func_name(msg->cmd);
	D("get func name %s", func_name);
	rpc_call_async(p, "MCP", func_name, str_input.buffer, str_input.len, rpc_response, agent); 
	free(args.CompactUri.buffer);
	D("start async rpc");
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

void free_string_ptr(char* str)
{
	if(str != NULL)
		free(str);
	str = NULL;
}

void release_rpc_message(rec_msg_t* msg)
{
	free_string_ptr(msg->option);
	free_string_ptr(msg->body);
	free_string_ptr(msg->user_context);
}

int agent_echo_read_test(pxy_agent_t *agent)
{
    rec_msg_t* msg;
	msg = calloc(sizeof(rec_msg_t), 1);
    parse_client_data(agent, msg);

    char *url;
    int ret = get_app_url(msg->cmd, 1, NULL, NULL, NULL, &url);
    D("url=%s", url);
    //get proxy uri
	if(send_rpc_server(msg, url, agent) < 0)
	{
    	release_rpc_message(msg);
        free(url);
		return -1;
	}

    free(url);
	pxy_agent_buffer_recycle(agent);
    release_rpc_message(msg);
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
pxy_agent_close(pxy_agent_t *agent)
{
	if(agent_epid != NULL)
		free(agent_epid);
    D("pxy_agent_close fired");
    buffer_t *b;

    if(agent->fd > 0){
	/* close(agent->fd); */
	if(ev_del_file_item(worker->ev,agent->fd) < 0){
	    D("del file item err, errno is %d",errno);
	}
	D("close the socket");
	close(agent->fd);
    }

    while(agent->buffer){
	b = agent->buffer;
	agent->buffer = b->next;

	if(b->data){
	    mp_free(worker->buf_data_pool,b->data);
	}
	mp_free(worker->buf_pool,b);
    }

    mp_free(worker->agent_pool,agent);
    D("pxy_agent_close finished");
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
		pxy_agent_close(agent);
	}
	return;

failed:
	D("failed!");
	pxy_agent_close(agent);
	map_remove(&worker->root, agent->epid);
	return;
}
