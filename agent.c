#include "proxy.h"
#include "agent.h"
#include "include/rpc_client.h"
#include "McpAppBeanProto.pb-c.h"
//#include "ClientHelper.c"

extern pxy_worker_t *worker;
size_t packet_len;
static char* PROTOCOL = "MCP/3.0";
char* agent_epid = NULL;
static int 
agent_send2(pxy_agent_t *agent,int fd)
{
    int i = 0;
    ssize_t n;
    size_t len;
    void *data;
    buffer_t *b = agent->buffer;
  
    n = agent->buf_parsed - agent->buf_sent;

    for(; b != NULL; agent->buf_sent += n, b = b->next, i++){
    
	if(i == 0){
	    data = (void*)((char*)b->data + agent->buf_sent);
	   len  = b->len - agent->buf_sent;
	}
	else{
	    data = b->data;
	    len  = b->len;
	}
	// call zookeeper get proxy send message
	/*	n = send(fd,data,len,0);
	D("n:%zu, fd #%d, errno :%d, EAGAIN:%d", n,fd,errno,EAGAIN);
      
	if(n < 0) {
	    if(errno == EAGAIN || errno == EWOULDBLOCK) {
		return 0;
	    }
	    else {
		pxy_agent_close(agent);
		pxy_agent_remove(agent);
		return -1;
	    }
	}
*/
	}
 //   pxy_agent_buffer_recycle(agent);

    return 0;
}

int
pxy_agent_downstream(pxy_agent_t *agent)
{
    return agent_send2(agent,agent->fd);
}

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
		D("Parse packet_len %d > agent->buf_len %lu", packet_len ,agent->buf_len);
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
		//	D("body[%d] is %d", t,msg->body[t]); 
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
	args->protocol = PROTOCOL;
 	args->cmd = msg->cmd;
	args->compacturi = Get_CompactUri(msg);
	args->userid = msg->userid;
	args->sequence = msg->seq;
	args->opt = msg->format;
	
	if(msg->user_context != NULL){
		args->usercontext.len = strlen(msg->user_context);
		args->usercontext.data = (uint8_t*)msg->user_context;
	}else{
		args->usercontext.len = 0; 
		args->usercontext.data = NULL;
	}

	if(msg->body != NULL){	
		args->content.len = msg->body_len;
		args->content.data = (uint8_t*)msg->body;
	}else{
		args->content.len = 0;
		args->content.data = NULL;
	}

	if(msg->epid != NULL)
		args->epid = msg->epid;
	else{
		args->epid = GenerateClientEpid("ANDROID", NULL);
		agent_epid = args->epid;
	}

	args->zipflag = msg->compress;
/*	
	D("args->protocol %s", args->protocol);
	D("args->cmd %u", args->cmd);
	D("args.compacturi %s", args->compacturi);
	D("args.userid %d", args->userid);
	D("args.sequence %d", args->sequence);
	D("args.opt %d", args->opt);
	D("args->usercontext.len %zu",args->usercontext.len);
	D("args->usercontext.len %zu", args->usercontext.len);
	unsigned int i = 0;
	D("args->content.len %zu", args->content.len);
	for(i=0; i < args->content.len ; i++)
		D("args->content.data %d", args->content.data[i]);
	D("args.epid %s", args->epid);
	D("args->zipflag %d", args->zipflag);*/
}	

int send_rpc_server(rec_msg_t* msg, char* proxy_uri, pxy_agent_t *agent)
{
	D("rpc uri %s", proxy_uri);
	rpc_stack_init("abc");
    rpc_client_t *client = rpc_client_new_full(proxy_uri, 128000);
	if(client == NULL){
		D("creat rpc client is NULL");
		return -1;
	}

	McpAppBeanProto args = MCP_APP_BEAN_PROTO__INIT; 
	get_rpc_arg(&args, msg);

	uint8_t *buf_out, *buf;
	buf = calloc(sizeof(args), 1);
	size_t buf_size = mcp_app_bean_proto__pack(&args, buf); 
	size_t buf_out_size = 0;
	rpc_int_t ret_code = rpc_client_send_request(client, "MCP", "mcore-Reg1V5", buf, buf_size,
				   	(void **)&buf_out, &buf_out_size);

	rpc_client_close(client);
    free_string_ptr(args.compacturi);
	free(buf);
	
	if(ret_code != RPC_CODE_OK || buf_out_size == 0){
		D("rpc return code %zu, buf_out_size %zu", ret_code, buf_out_size);
		return -1;
	}

	D("rpc return code success %zu", ret_code);
    rec_msg_t* rec_msg;
	if(process_received_msg(buf_out_size, buf_out, rec_msg) < 0){
		rpc_free(buf_out);
		return -1;
	}

	agent_send_client(rec_msg, agent);
	rpc_free(buf_out);
	return 0;
}

void agent_send_client(rec_msg_t* rec_msg, pxy_agent_t *agent)
{
	// send to client
	int len = 0;
    char* data =  GetData(rec_msg, len);
	send(agent->fd, data, len, 0);
}

int process_received_msg(size_t buf_size, uint8_t* buf_ptr, rec_msg_t* msg)
{
	McpAppBeanProto *ret = mcp_app_bean_proto__unpack(NULL, buf_size, buf_ptr);
	if(ret == NULL)
	{
		D("rpc unpacket fail");
		return -1;
	}
	msg->cmd = ret->cmd;
	msg->body_len = ret->content.len;
	msg->body = (char*)ret->content.data;
	msg->userid = ret->userid; 
	msg->seq = ret->sequence;
	msg->format = ret->opt;
	msg->user_context_len = ret->usercontext.len;
	msg->user_context = (char*)ret->usercontext.data;
	msg->compress = ret->zipflag;
	msg->epid = ret->epid;

	/*	int i = 0;
	D("msg->cmd %d", msg->cmd);
	D("msg->body_len %d", msg->body_len);
	for(i = 0; i < msg->body_len; i++)
		D("msg->body[%d] = %d", i, (unsigned char) *(msg->body+i));
	D("msg->userid %d", msg->userid);
	D("msg->seq %d", msg->seq);
	D("msg->format %d", msg->format);
	D("msg->user_context_len %d", msg->user_context_len);
	for(i = 0; i < msg->user_context_len; i++)
		D("msg->user_context[%d] = %d", i, (unsigned char) *(msg->user_context+i));
	D("msg->compress %d", msg->compress);
	D("msg->epid %s", msg->epid);*/
	return 0;
}	

void free_string_ptr(char* str)
{
	if(str != NULL)
	{
		free(str);
		str = NULL;
	}	
}

void release_rpc_message(rec_msg_t* msg)
{
	free_string_ptr(msg->option);
	free_string_ptr(msg->body);
	free_string_ptr(msg->user_context);
	return 0;
}

int agent_echo_read_test(pxy_agent_t *agent)
{
    rec_msg_t* msg;
	msg = calloc(sizeof(rec_msg_t), 1);
    parse_client_data(agent, msg);
	
	//get proxy uri
	if(send_rpc_server(msg, "tcp://192.168.110.182:6101", agent) < 0)
		return -1;
	pxy_agent_buffer_recycle(agent);
    release_rpc_message(msg);
	pxy_agent_remove(agent);
	pxy_agent_close(agent);
    return 0;
}

/*
int pxy_agent_data_received(pxy_agent_t *agent)
{
    // *
    // * parse the data, and send the packet to the upstream
    // *
    // * proto format:
    // * 00|len|cmd|content|00
    // *

    int idx,n,i = 0;
    char *c;

    n = agent->buf_offset - agent->buf_sent;

    if(n){
	idx = agent->buf_sent;

	int cmd = -1,len = 0,s = 0;
    
	while((i++ < n) && (c=buffer_read(agent->buffer,idx)) != NULL) {
      
	  // parse state machine
	    switch(s){
	    case 0:
		if(*c != 0)
		    return -1;
		s++; idx++;
		break;
	    case  1:
		len = (int)*c;
		s++; idx++;
		break;
	    case  2:
		cmd = (int)*c;
		s++; idx += len-3;
		break;
	    case  3:
		if(*c !=0)
		    return -1;

		//on message parse finish
		s=0; idx++;
		agent->buf_parsed = idx;
		break;
	    }
	}

	pxy_agent_upstream(cmd,agent);
    }

    return 0;
}*/

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

int pxy_agent_upstream(int cmd, pxy_agent_t *agent)
{
    return agent_send2(agent,worker->bfd);
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
			pxy_agent_remove(agent);
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
			break;
	}
	D("buffer-len %zu", agent->buf_len);
	if(agent_echo_read_test(agent) < 0)
		pxy_agent_close(agent);
	return;

failed:
	pxy_agent_close(agent);
	pxy_agent_remove(agent);
	return;
}
