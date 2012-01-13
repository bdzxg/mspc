/*
 * Copyright liulijin<llj098@gmail.com>
 */ 

#include "proxy.h"

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
    config->worker_count = 4;

    return 1;
  }

  return -1;
}

int 
pxy_init_master()
{
  int s;
  struct sockaddr_in addr1;

  if(pxy_init_config()){
    
    master = (pxy_master_t*)malloc(sizeof(*master));
    master->config = config;

    master->listen_fd =socket(AF_INET,SOCK_STREAM,0);
    if(master->listen_fd < 0)
      return -1;

    setnonblocking(master->listen_fd);

    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(config->client_port);
	
    s = bind(master->listen_fd, (struct sockaddr*)&addr1, sizeof(addr1));

    if(s < 0)
      return -1;

    if(listen(master->listen_fd,1000) < 0)
      return -1;
  }
 
  return -1;
}

int 
pxy_init_worker()
{
  worker = (pxy_worker_t*)malloc(sizeof(*worker));
  if(worker) {
    worker->ev = ev_create();
    worker->agent_pool = mp_create(sizeof(pxy_agent_t),0,"AgentPool");
    worker->buf_data_pool = mp_create(BUFFER_SIZE,0,"BufDataPool");
    worker->buf_pool = mp_create(sizeof(buffer_t),0,"BufPool");
    
    
    if(worker->ev != NULL)
      return 1;
    else
      return -1;
  }

  return -1;
}

void
pxy_worker_client_rfunc(ev_t* ev,ev_file_item_t* fi)
{
  int iovn,i = 0,f,existn,readn=0;
  buffer_t *buffer,*bh = NULL;
  void *d = NULL;
  pxy_agent_t *agent = NULL;

  if(fi->fd == master->listen_fd){
    for(i=0;i<100;i++){
      /*try to accept 100 times*/
      socklen_t sin_size = sizeof(master->addr);
      f = accept(master->listen_fd,&(master->addr),&sin_size);
      if(f>0){

	/* FIXME:maybe we should try best to accept and 
	 * delay add events */
	setnonblocking(f);
	agent = pxy_agent_new(worker->agent_pool,f,0,NULL);
	ev_file_item_new(f,agent,pxy_worker_client_rfunc,NULL,EV_READABLE);
      }
      else{
	break;
      }
    }
  }
  else{

    agent = fi->data;
    if(!agent){
      W("fd has no agent,ev->data is NULL,close the fd");
      close(fi->fd);
      return;
    }

    if( ioctl(fi->fd,FIONREAD,&readn) >0 && readn > 0) {
      
      existn = agent->buf_offset % BUFFER_SIZE;
      agent->buf_offset += readn;

      if(existn > 0){
	iovn = 1;
	readn -= BUFFER_SIZE - existn;
      }

      iovn += readn / BUFFER_SIZE + (((readn % BUFFER_SIZE) > 0) ? 1 : 0);

      struct iovec iov[iovn];

      if(existn > 0 && agent->buffer){

	bh = list_entry(&(agent->buffer->list.prev), buffer_t, list);
	d = (void*)((char*)bh->data + existn);

	iov_init(&(iov[0]), d, BUFFER_SIZE - existn);
	readn -= BUFFER_SIZE;
	i = 1;
      }

      for(; i < iovn ;i++){

	buffer = buffer_fetch(worker->buf_pool,worker->buf_data_pool);

	if(i == 0){
	  bh = buffer;
	}
	
	list_append(&buffer->list,&bh->list);

	iov_init(&(iov[i]), buffer->data, BUFFER_SIZE);
	readn -= BUFFER_SIZE;
      }
      
      readn = readv(fi->fd,iov,iovn);
      if(readn){
	//handle received data
	if(!agent->buffer){
	  agent->buffer = bh;
	}
	list_append(&bh->list,&agent->buffer->list);
      }

    }
  }
}


int 
pxy_start_worker()
{
  ev_file_item_t* fi ;
  int fd = master->listen_fd;

  fi = ev_file_item_new(fd, worker, pxy_worker_client_rfunc, NULL, EV_READABLE);

  if(!fi){
    goto start_failed;
  }

  worker->bfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if(!worker->bfd){
    goto start_failed;
  }
  
  if(!connect(worker->bfd,(struct sockaddr*)worker->baddr,
	      sizeof(*(worker->baddr)))){
    goto start_failed;
  }
    
  return 1;

 start_failed:
  return -1;
}


int 
create_shm()
{
  int p = shmget(IPC_PRIVATE,sizeof(4),0600/*user read&writer*/);
  char* ptr;
  if(p>0) {
    printf("shm created, id %d\n",p);
    ptr = shmat(p,0,0);
    ptr = "123";
    printf("%s\n",ptr);
  }
  
  return p;
}

int 
main(int len,char** args)
{
  /* char p[80]; */
  int s,i=0;

  s = pxy_init_master();

  if(!s)
    return -1;
 

  /*spawn worker*/

  for(;i<config->worker_count;i++){

    pid_t p = fork();

    if(p < 0) {
      printf("%s","forkerror");
    }
    else if(p == 0){/*child*/
      printf("%d\n",getpid());
      if(pxy_init_worker())
	pxy_start_worker();
    }
    else{/*parent*/
      /*I("%d\n",getpid());*/
    }
  }

  /*while(scanf("%s",p) >= 0 && strcmp(p,"quit") !=0){}*/
  return 1;
}
