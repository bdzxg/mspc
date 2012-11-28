/*
 * Copyright liulijin<llj098@gmail.com>
 */ 

#include "proxy.h"
#include "agent.h"

pxy_master_t* master;
pxy_config_t* config;
pxy_worker_t* worker;
extern freeq_t *request_q;
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

void set2buf(char** buf, char* source, int len)
{
	int j;
	for(j = 0; j < len; j++) {
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
	
	char tmp[102400] = {0};
	to_hex(t->body, t->body_len, tmp);

	D("body is %s", tmp);

	memcpy(ret + 24, t->body, t->body_len);
	return ret;
}

int main()
{
	D("process start");
	char ch[80];
	pxy_worker_t *w;
	log_file = stdout;

	if(pxy_init_master() < 0){
		E("master initialize failed");
		return -1;
	}
	D("master initialized");


	/* TODO: Maybe we need remove the master-worker mode,
	   seems useless
	 */

	if(worker_init()<0) {
		E("worker #%d initialized failed" , getpid());
		return -1;
	}
	D("worker inited");

	if(worker_start() < 0) {
		E("worker #%d started failed", getpid()); 
		return -1;
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
