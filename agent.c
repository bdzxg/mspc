#include "proxy.h"

extern pxy_worker_t *worker;

int
pxy_agent_send(pxy_agent_t *agent,int fd)
{
  int iovn,p,s,i,writen,n;
  buffer_t *b = agent->buffer;

  iovn = 1;
  p = agent->buf_parsed;
  s = agent->buf_sent;
  n = p - s;

  
  if(n <= 0){
    D("nothing to send");
    return 0;
  }
  
  /*calculate the iovn */
  while(n > BUFFER_SIZE){ iovn++; n -= BUFFER_SIZE; }

  /*calcute the buffer start pos */
  for(i=0 ; i< s / BUFFER_SIZE ; i++) { b = buffer_next(b); }


  struct iovec iov[iovn];
  
  for(i=0; i<iovn; i++,b=buffer_next(b)){

    if(i == 0){
      int offset = s % BUFFER_SIZE;
      iov[i].iov_base = ((char *)(b->data)) + offset;
      iov[i].iov_len = (BUFFER_SIZE - offset) > n ? n : (BUFFER_SIZE - offset);
      D("IOV.LEN:%d",iov[i].iov_len);
      continue;
    }

    if(i == iovn){
      iov[i].iov_base = b->data;
      iov[i].iov_len = p % BUFFER_SIZE;
      continue;
    }
    
    iov[i].iov_base = b->data;
    iov[i].iov_len = BUFFER_SIZE;
  }
  
  writen = writev(fd,iov,iovn);
  if(writen > 0) {
    agent->buf_sent += writen;
    D("data to send is :%d, iovn:%d,writen:%d", n, iovn,writen);
  }
  
  if(writen > 0){
    pxy_agent_buffer_recycle(agent,writen);
  }

  return writen;
} 


int
pxy_agent_downstream(pxy_agent_t *agent)
{
  return pxy_agent_send(agent,agent->fd);
}


int 
pxy_agent_echo_test(pxy_agent_t *agent)
{
  char *c;
  int idx,n,i = 0;

  n = agent->buf_offset - agent->buf_sent;
  D("n is :%d", n);

  if(n){ 
    idx = agent->buf_sent;
    while((i++ < n) && (c=buffer_read(agent->buffer,idx)) != NULL) {
     
      if(*c == 'z' && (i + 2) <= n) {
	
	char *c1 = buffer_read(agent->buffer,idx+1);
	char *c2 = buffer_read(agent->buffer,idx+2);

	if(*c1 == '\r' && *c2 =='\n'){
	  agent->buf_parsed = idx+2+1;
	  i+=2; idx+=2;
	}
      }

      idx++;
   }
    
    D("the agent->offset:%d,agent->sent:%d,agent->parsed:%d",
      agent->buf_offset,
      agent->buf_sent,
      agent->buf_parsed);
      
    if(pxy_agent_downstream(agent) < 0){
      pxy_agent_remove(agent);
      pxy_agent_close(agent);
    }
  }

  return 0;
}


int
pxy_agent_data_received(pxy_agent_t *agent)
{
  /*
   * parse the data, and send the packet to the upstream
   *
   * proto format:
   * 00|len|cmd|content|00
   */

  int idx,n,i = 0;
  char *c;

  n = agent->buf_offset - agent->buf_sent;

  if(n){
    idx = agent->buf_sent;

    int cmd = -1,len = 0,s = 0;
    
    
    while((i++ < n) && (c=buffer_read(agent->buffer,idx)) != NULL) {
      
      /*parse state machine*/
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

	/*on message parse finish*/
	s=0; idx++;
	agent->buf_parsed = idx;
	break;
      }
    }

    pxy_agent_upstream(cmd,agent);
  }

  return 0;
}


int 
pxy_agent_buffer_recycle(pxy_agent_t *agent,int n)
{
  if(!agent)
    return -1;

  int rn;
  
  while(n > BUFFER_SIZE){
    list_remove(&agent->buffer->list);
    buffer_release(agent->buffer,worker->buf_pool,worker->buf_data_pool);
    rn += BUFFER_SIZE;
    agent->buf_sent -= BUFFER_SIZE;
    agent->buf_offset -= BUFFER_SIZE;
    agent->buf_parsed -= BUFFER_SIZE;
  }

  
  return rn;
}

void 
pxy_agent_close(pxy_agent_t *agent)
{
  D("pxy_agent_close fired");
  buffer_t *b;

  if(agent->fd > 0){
    /* close(agent->fd); */
    D("shutdown the socket");
    shutdown(agent->fd,SHUT_RDWR);
  }

  if(agent->buffer) {
    buffer_for_each(b,agent->buffer){
      if(b){
	if(b->data){
	  mp_free(worker->buf_data_pool,b->data);
	}
	mp_free(worker->buf_pool,b);
      }
    }

    mp_free(worker->buf_pool,agent->buffer);
  }

  mp_free(worker->agent_pool,agent);
}

int 
pxy_agent_prepare_buf(pxy_agent_t *agent,struct iovec *iov,int iovn)
{
  return -1;
}


int
pxy_agent_upstream(int cmd,pxy_agent_t *agent)
{
  return pxy_agent_send(agent,worker->bfd);
}


pxy_agent_t *
pxy_agent_new(mp_pool_t *pool,int fd,int userid)
{
  pxy_agent_t *agent = mp_alloc(pool);
  if(!agent){
    D("no mempry for agent"); 
    goto failed;
  }

  agent->buffer = mp_alloc(worker->buf_pool);
  if(!agent->buffer) {
    D("no memory for buffer");
    goto failed;
  }
  INIT_LIST_HEAD(&(agent->buffer->list));

  agent->fd         = fd;
  agent->user_id    = userid;
  agent->buf_sent   = 0;
  agent->buf_parsed = 0;
  agent->buf_offset = 0;
  agent->buf_list_n = 0;

  return agent;

 failed:
  if(agent){
    mp_free(worker->agent_pool,agent);
  }
  return NULL;
}

void
agent_recv_client(ev_t *ev,ev_file_item_t *fi)
{
  int n;
  buffer_t *b;
  pxy_agent_t *agent = fi->data;

  if(!agent){
    W("fd has no agent,ev->data is NULL,close the fd");
    close(fi->fd); return;
  }

  while(1) {

    b = buffer_fetch(worker->buf_pool,worker->buf_data_pool);
    if(!b) {
      D("no buf available"); return;
    }

    buffer_append(b,agent->buffer);


    n = recv(fi->fd,b,BUFFER_SIZE,0);

    if(n < 0){
      if(errno == EAGAIN || errno == EWOULDBLOCK) {
	break;
      }
     else {
	D("read error,errno is %d",errno);
	goto failed;
      }
    }

    if(n == 0){
      D("socket fd #%d closed by peer",fi->fd);
	goto failed;
    }

    if(n < BUFFER_SIZE) {
      break;
    }
  }

  if(pxy_agent_echo_test(agent) < 0){
    pxy_agent_close(agent);
  }

 failed:
  if(b) {
    buffer_release(b,worker->buf_pool,worker->buf_data_pool);
  }

  pxy_agent_close(agent);
  pxy_agent_remove(agent);
  return;
}

