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
FILE* log_file;

int 
pxy_init_config()
{
	config = (pxy_config_t*)pxy_calloc(sizeof(*config));

	if(config){
		config->client_port = 8014;
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
		free(c);
		D("send failed%d",a);
		return  -1 ;
	}

	free(c);
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

	int reuse = 1;
	setsockopt(master->listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	
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

pxy_agent_t* get_agent(char* key)
{
	pxy_agent_t* ret = NULL;
	ret = map_search(&worker->root, key);
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
	// option use 0 template
	set2buf(&rval, (char*)&padding, 4);
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
	return ret;
}

void process_bn(rec_msg_t* msg, pxy_agent_t* a)
{
	if(msg->cmd == 105)
		// TODO add tcp flow
		return;
	// can not be 102
	int len;
	char* data = get_send_data(msg, &len);
	send_to_client(a, data, &len);
	free(data);
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
	msg.format = input.Opt;
	msg.compress = input.ZipFlag;
	msg.epid = calloc(input.Epid.len+1, 1); 
	memcpy(msg.epid, input.Epid.buffer, input.Epid.len);
	//D("bn epid %s cmd %d userid %d", msg.epid, msg.cmd, msg.userid);
	pxy_agent_t* a = NULL;
	a = get_agent(msg.epid);
	if(a == NULL) {
		if(msg.epid != NULL) {
			D("BN cann't find epid %s connection!!!", msg.epid);
		}
		free(msg.epid);
		return;
	}
	msg.seq = a->bn_seq++;
	process_bn(&msg, a);
	W("BN epid %s!", msg.epid);
	free(msg.epid);

	retval output;
	output.option.len = input.Protocol.len;
	output.option.buffer = input.Protocol.buffer;

	rpc_pb_string str_out;
	int outsz = 256;  
	str_out.buffer = rpc_connection_alloc(c, outsz);
	str_out.len = outsz;
	r = rpc_pb_pattern_pack(rpc_pat_retval, &output, &str_out);

	if (r >= 0) 
	{
 		 rpc_return(c, str_out.buffer, str_out.len);
	}
	else
		rpc_return_error(c, RPC_CODE_SERVER_ERROR, "output encode failed!");
}

int rpc_server_init()
{
	pthread_t id;
	if(pthread_create(&id, NULL, (void*)rpc_server_thread, NULL))
		return -1;
	else 
		return 0;	
}	
 
int reg3_recycle_init()
{
	pthread_t id;
	if(pthread_create(&id, NULL, (void*)recycle_connection_reg3_thread, NULL))
		return -1;
	else 
		return 0;	
}

void* rpc_server_thread(void* args)
{
	UNUSED(args);

	rpc_server_t* s = rpc_server_new();
	//s->is_main_dispatch_th = 0;
	rpc_server_regchannel(s, LISTENERPORT);
	rpc_server_regservice(s, "IMSPRpcService", "ReceiveMessage", receive_message);
	rpc_args_init();

	D("rpc server start!");
	if(rpc_server_start(s)!=RPC_OK) {
		D("init rpc server fail");
	}
	else {
		D("init rpc server ok");
	}
	return NULL;
}

void* recycle_connection_reg3_thread(void* args)
{
	UNUSED(args);
	while(1)
	{
		worker_recycle_reg3();		
		sleep(90*1000);
	}		
}

int main()
{
	char ch[80];
	pxy_worker_t *w;
	log_file = stdout;
	
	if(pxy_init_master() < 0){
		E("master initialize failed");
		return -1;
	}
	D("master initialized");


	/*remove the master-worker model*/
	if(worker_init()<0) {
		E("worker #%d initialized failed" , getpid());
		return -1;
	}
	D("worker inited");


	if(rpc_server_init() < 0) {
		E("rpc server start failed");
		return -1;
	}
	D("rpc server inited");
	if(reg3_recycle_init()<0)
	{
		E("reg3 recycle init start failed");
		return -1;
	}
	D("reg3 recycle inited");

	if(!worker_start()) {
		D("worker #%d started failed", getpid()); return -1;
	}
	D("worker started");
		

	while(scanf("%s",ch) >= 0 && strcmp(ch,"quit") !=0){ 
	}

	w = (pxy_worker_t*)master->workers;
	pxy_send_command(w,PXY_CMD_QUIT,-1);

	sleep(5);
	D("pxy_master_close");
	pxy_master_close();
	return 1;
}
