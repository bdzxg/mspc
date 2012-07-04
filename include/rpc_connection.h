
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_CONNECTION_H_INCLUDED_
#define _RPC_CONNECTION_H_INCLUDED_

#include "rpc_types.h"

BEGIN_DECLS

typedef enum{
  rpc_cs_read, //connection need to read data
  rpc_cs_process, //connection need to process data
  rpc_cs_write, //connection need to write data
  rpc_cs_error  //connection need to closed
}rpc_connection_state;

typedef enum{
  rpc_ct_s,     //connection for server
  rpc_ct_c_st,  //connection for client st
  rpc_ct_c_mt,  //connection for client mt
}rpc_connection_type;

//open -> connecting -> connected -> closing -> closed
typedef enum{
  rpc_st_open, 
  rpc_st_connecting,
  rpc_st_connected,
  rpc_st_closing,
  rpc_st_closed
}rpc_connection_status;

struct rpc_connection_s {
	int fd;
  int events;
  int revents;
  //int idx;

  int num_alloc;
  rpc_msec_t idle_time;
  rpc_connection_state state;
  rpc_connection_type type; 
  rpc_connection_status status;

  struct sockaddr *sockaddr;
  socklen_t socklen;

	struct ev_loop *loop;
	struct ev_io watcher;
  struct ev_async notify;
  struct ev_timer timer;

  rpc_pool_t *pool; //used for alloc after sended need to free
  rpc_buf_t *r_buf; //used for read data
  rpc_buf_t *w_buf; //not use now
  rpc_thread_worker_t *thread;

  struct iovec *iov;
  int iovsize;
  int iovused;
  int iovcurr;

  struct msghdr *msglist;
  int    msgsize;   
  int    msgused;  
  int    msgcurr; 

  size_t msgbytes; 
  size_t recvcount; 
  size_t recvbytes;
  size_t sendcount;
  size_t sendbytes;

  rpc_queue_t *req_pending; 

  void *owner; //channel or proxy
  void *data; //req or rsp

  u_char is_addnotify;
  u_char is_addev;

};

rpc_int_t rpc_connection_init();

rpc_connection_t* rpc_connection_get(int fd,rpc_connection_state st,rpc_connection_type tp,
    rpc_thread_worker_t *thread, void *owner, rpc_msec_t idle_time,
    struct ev_loop *loop, int events);

#define rpc_connection_notify(c) do {   \
  if (c->loop != NULL) {                \
    ev_async_send(c->loop, &c->notify); \
  }                                     \
} while (0)

#define rpc_connection_update_ev(c, evts) do { \
  if (c->events == (evts))                     \
    break;                                     \
  ev_io_stop(c->loop, &c->watcher);            \
  ev_io_set(&c->watcher, c->fd , evts);        \
  ev_io_start(c->loop, &c->watcher);           \
  c->events = (evts);                            \
} while (0)

#define rpc_connection_add_ev(c) do { \
  if (c->is_addev) { \
    break; \
  } \
  if (c->idle_time != RPC_TIMER_INFINITE) {  \
    c->timer.data = c; \
    ev_init(&c->timer, rpc_connection_timeout); \
    c->timer.repeat = c->idle_time; \
    ev_timer_again(c->loop,&c->timer); \
  } \
	c->watcher.data = c; \
	ev_io_init(&c->watcher,rpc_connection_process,c->fd,c->events); \
	ev_io_start(c->loop, &c->watcher); \
  c->is_addev = 1; \
} while (0)

#define rpc_connection_add_notify(c) do { \
  if (c->is_addnotify) { \
    break; \
  } \
  c->notify.data = c; \
  ev_async_init(&c->notify, rpc_connection_pnotify); \
  ev_async_start(c->loop, &c->notify); \
  c->is_addnotify = 1; \
} while (0)

#define rpc_connection_set_fd(c, vfd) { \
  c->status = rpc_st_connected;           \
  c->fd = vfd;                             \
  rpc_connection_add_ev(c);               \
} while (0)

void *rpc_connection_alloc(rpc_connection_t *c, size_t sz);

void *rpc_connection_copy(rpc_connection_t *c, void *data, size_t sz);

void rpc_connection_free(rpc_connection_t *c);

void rpc_connection_close(rpc_connection_t *c);

rpc_int_t rpc_connection_sendmsg(rpc_connection_t *c);

void rpc_connection_sendreq(rpc_connection_t *c, rpc_request_t *req);

void rpc_connection_cbreq(rpc_connection_t *c, rpc_int_t code);

void rpc_connection_timeout(struct ev_loop *l, struct ev_timer *timer, int revents);

void rpc_connection_pnotify(struct ev_loop *l, struct ev_async *notify, int revents);

void rpc_connection_process(struct ev_loop *l,struct ev_io *watcher, int revents);

void dump_buffer(void *data, size_t size);
END_DECLS
#endif
