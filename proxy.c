/*
 * Copyright liulijin<llj098@gmail.com>
 */ 

#include "proxy.h"
#include "agent.h"
#include "include/rpc_client.h"
#include "include/rpc_server.h"
#include "include/rpc_args.c"

pxy_master_t* master;
pxy_config_t* config;
pxy_worker_t* worker;

int 
pxy_init_config()
{
    config = (pxy_config_t*)pxy_calloc(sizeof(*config));

    if(config){
	config->client_port = 9000;
	config->backend_port = 9001;
	config->worker_count = 1;

	return 1;
    }

    return -1;
}

void
pxy_master_close()
{
    if(master->listen_fd > 0){
	D("close the listen fd");
	close(master->listen_fd);
    }
  
    /* TODO: Destroy the mempool */
}

int
pxy_send_command(pxy_worker_t *w,int cmd,int fd)
{
    struct msghdr m;
    struct iovec iov[1];
    pxy_command_t *c = malloc(sizeof(*c));

    if(!c) {
	D("no memory");
	return -1;
    }

    c->cmd = cmd;
    c->fd = fd;
    c->pid = w->pid;

    iov[0].iov_base = c;
    iov[0].iov_len = sizeof(*c);

    m.msg_name = NULL;
    m.msg_namelen = 0;
    m.msg_control = NULL;
    m.msg_controllen = 0;
  
    m.msg_iov = iov;
    m.msg_iovlen = 1;

    int a;
    if((a=sendmsg(w->socket_pair[0],&m,0)) < 0) {
	D("send failed%d",a);
	return  -1 ;
    }

    D("master sent cmd:%d to pid:%d fd:%d", cmd, w->pid, fd);
    return 0;
}

int 
pxy_start_listen()
{
    struct sockaddr_in addr1;

    master->listen_fd = socket(AF_INET,SOCK_STREAM,0);
    if(master->listen_fd < 0){
	D("create listen fd error");
	return -1;
    }

    if(setnonblocking(master->listen_fd) < 0){
	D("set nonblocling error");
	return -1;
    }

    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(config->client_port);
    addr1.sin_addr.s_addr = 0;
	
    if(bind(master->listen_fd, (struct sockaddr*)&addr1, sizeof(addr1)) < 0){
	D("bind error");
	return -1;
    }

    if(listen(master->listen_fd,1000) < 0){
	D("listen error");
	return -1;
    }
    
    return 0;
}

int 
pxy_init_master()
{
    if(!pxy_init_config()){
	D("config initialize error");
	return -1;
    }
 
    master = (pxy_master_t*)malloc(sizeof(*master));
    if(!master){
	D("no memory for master");
	return -1;
    }
    master->config = config;
    master->workers = 
	(pxy_worker_t**)malloc(config->worker_count * sizeof(pxy_worker_t));

    if(!master->workers) {
	D("no memory for workers");
	return -1;
    }

    return pxy_start_listen();
}

pxy_agent_t* get_agent(rec_msg_t* msg)
{
	pxy_agent_t* ret = NULL;
	ret = map_search(&worker->root,msg->epid);
	return ret;
}

void set2buf(char** buf, char* source, int len)
{
	int j;
	for(j = 0; j < len; j++)
	{
		**buf = *(source+j);	
		(*buf)++;
	}
}

char* get_send_data(rec_msg_t* t, int* length)
{
	int padding = 0;
	int offset = 24;
	*length = 25 + t->body_len;
	char* ret = calloc(sizeof(char), *length);
	if(ret == NULL)
		return ret;

	t->version = 0;//msp use 0.
	char* rval = ret;
	set2buf(&rval, (char*)&padding, 1);
    // length
	set2buf(&rval, (char*)length, 2);	
	// version
	set2buf(&rval, (char*)&t->version, 1);
	// userid
	set2buf(&rval, (char*)&t->userid, 4);
	// cmd
	set2buf(&rval, (char*)&t->cmd, 2);
	// seq
	set2buf(&rval, (char*)&t->seq, 2);
	// offset
	set2buf(&rval, (char*)&offset, 1);
	// format
	set2buf(&rval, (char*)&t->format, 1);
	// zipflag compress
	set2buf(&rval, (char*)&t->compress, 1);
	// clienttype
	set2buf(&rval, (char*)&t->client_type, 2);
	// clientversion
	set2buf(&rval, (char*)&t->client_version, 2);
	// 0
	set2buf(&rval, (char*)&padding, 1);
	// option
	set2buf(&rval, (char*)&t->format, 4);
    /*
	 int i;
	 if(t->option_len != 0)
	 for(i = 0; i < t->option_len; i++)
			*(rval++) = *(t->option++);
	 else rval += 4;*/

	// body
    int i;
	for(i = 0; i < t->body_len; i++ )
		*(rval++) = *(t->body++);
	// end padding 0
	*rval = 0;
	/*rval = ret;
	for(i = 0; i < *length; i++)
	{
		D("content %d", (unsigned char)*(rval++));
	}*/
	return ret;
}

void process_bn(rec_msg_t* msg)
{
	pxy_agent_t* ret = NULL;
	if(msg->cmd == 105)
		// TODO add tcp flow
		return;
	else 
		ret = get_agent(msg);

	if(ret != NULL)
	{
		/*  can not be 102
		if(msg->cmd == 102 && msg->user_context!= NULL){
			// record usercontextbytes	}*/
		int len;
		char* data = get_send_data(msg, &len);
		send(ret->fd, data, len, 0);
	}
	else
	{
		D("BN cann't find connection!!!");
	}
}

void receive_message(rpc_connection_t *c,void *buf, size_t buf_size)
{
		McpAppBeanProto input;
		rpc_pb_string str = {buf, buf_size};
		int r = rpc_pb_pattern_unpack(rpc_pat_mcpappbeanproto, &str, &input);

		if ( r < 0) {
				rpc_return_error(c, RPC_CODE_INVALID_REQUEST_ARGS, "input decode failed!");
				return;
		}

		rec_msg_t msg;
		msg.cmd = input.Cmd;
		msg.body_len = input.Content.len;
		msg.body = input.Content.buffer;
		msg.userid = input.UserId;
		msg.seq = input.Sequence;
		msg.format = input.Opt;
		msg.user_context = input.UserContext.buffer;
	    msg.user_context_len = input.UserContext.len;	
		msg.compress = input.ZipFlag;
		msg.epid = calloc(input.Epid.len+1, 1); 
		memcpy(msg.epid, input.Epid.buffer, input.Epid.len);

		process_bn(&msg);
		free(msg.epid);
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
		} else {
				rpc_return_error(c, RPC_CODE_SERVER_ERROR, "output encode failed!");
		}
}

int rpc_server_init()
{
	pthread_t id;
	if(pthread_create(&id, NULL, (void*)rpc_server_thread, NULL))
		return -1;
	else 
		return 0;	
}	
 
void* rpc_server_thread(void* args)
{
	rpc_server_t* s = rpc_server_new();
	//s->is_main_dispatch_th = 0;
	rpc_server_regchannel(s, "tcp://0.0.0.0:9999");
	rpc_server_regservice(s, "IMSPRpcService", "ReceiveMessage", receive_message);
	rpc_args_init();
	if(rpc_server_start(s)!=RPC_OK)
		D("init rpc server fail");
	else
		D("init rpc server ok");
}

//int main(int len,char** args)
int main()
{
    /* char p[80]; */
    int i=0;
    char ch[80];
    pxy_worker_t *w;
	
    if(pxy_init_master() < 0){
	D("master initialize failed");
	return -1;
    }
    D("master initialized");

    /*spawn worker*/

    for(;i<config->worker_count;i++){
  
	w = (pxy_worker_t*)(master->workers + i);

	if(socketpair(AF_UNIX,SOCK_STREAM,0,w->socket_pair) < 0) {
	    D("create socket pair error"); continue;
	}

	if(setnonblocking(w->socket_pair[0]) < 0) {
	    D("setnonblocking error fd:#%d",w->socket_pair[0]); continue;
	}
 
	if(setnonblocking(w->socket_pair[1]) < 0) {
	    D("setnonblocking error fd:#%d",w->socket_pair[1]); continue;
	}

	pid_t p = fork();

	if(p < 0) {
			D("%s","forkerror");
	}
	else if(p == 0){/*child*/
		if(worker_init()<0){
			D("worker #%d initialized failed" , getpid());
			return -1;
		}
		D("worker #%d initialized success", getpid());
		D("socket_pair[0]=%d,socket_pair[1]=%d",w->socket_pair[0],w->socket_pair[1]);
		//rpc_server_init();
		close (w->socket_pair[0]); /*child should close the pair[0]*/

		ev_file_item_t *f = ev_file_item_new(w->socket_pair[1],
						worker,
						worker_recv_cmd,
						NULL,
						EV_READABLE | EPOLLET);
		if(!f){ 
			D("new file item error"); return -1; 
		}
		if(ev_add_file_item(worker->ev,f) < 0) {
			D("add event error"); return -1;
		}
			
		if(!worker_start()) {
			D("worker #%d started failed", getpid()); return -1;
		}
	}
	else{ /*parent*/
		w->pid = p;
		D("close worker");
		close(w->socket_pair[1]); /*parent close the pair[1]*/
	}
	}
	D("Pid = %d",getpid());
	while(scanf("%s",ch) >= 0 && strcmp(ch,"quit") !=0){ 
	}

	w = (pxy_worker_t*)master->workers;
	pxy_send_command(w,PXY_CMD_QUIT,-1);

	sleep(5);
	D("pxy_master_close");
	pxy_master_close();
	return 1;
}

