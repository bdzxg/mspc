
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_EVENT_H_INCLUDED_
#define _RPC_EVENT_H_INCLUDED_

#include "rpc_types.h"
#include "rpc_timer.h"

#define RPC_EVENT_READ   EPOLLIN
#define RPC_EVENT_WRITE  EPOLLOUT

/*  ET  epoll */
#define RPC_EVENT_CLEAR  EPOLLET

/*  LT  select or poll */
#define RPC_EVENT_LEVEL  0
#define RPC_EVENT_CLOSE  0x99

typedef void (*rpc_event_handler_pt)(rpc_event_t *ev);

struct rpc_event_s {
  void *data;
  rpc_event_handler_pt handler;
  unsigned write:1;
  unsigned accept:1;

  /* used to detect the stale events in epoll */
  unsigned instance:1;

  /* the event was passed or would be passed to a kernel; */
  unsigned active:1;
  unsigned disable:1;
  unsigned eof:1;
  unsigned ready:1;
  unsigned available:1;
  unsigned closed:1;
  unsigned error:1;
  unsigned timerout:1;

  rpc_rbtree_node_t  timer;
  unsigned timer_set; 
};

/*
typedef ssize_t (*rpc_recv_pt)(rpc_connection *c,u_char *buf,size_t size);

typedef ssize_t (*rpc_send_pt)(rpc_connection *c,u_char *buf,size_t size);
*/
rpc_int_t rpc_event_init();

void rpc_event_done();

rpc_int_t rpc_event_process(rpc_msec_t timer);

rpc_int_t rpc_event_add(rpc_event_t *ev,rpc_int_t event,rpc_uint_t flags);

rpc_int_t rpc_event_del(rpc_event_t *ev,rpc_int_t event,rpc_uint_t flags);

rpc_int_t rpc_event_add_connection(rpc_connection_t *c);

rpc_int_t rpc_event_del_connection(rpc_connection_t *c,rpc_uint_t flags);

#endif
