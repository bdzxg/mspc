
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_TIMER_H_INCLUDED_
#define _RPC_TIMER_H_INCLUDED_

#include "rpc_rbtree.h"
#include "rpc_event.h"

#define RPC_TIMER_INFINITE (rpc_msec_t) -1

#define RPC_TIMER_LAZY_DELAY  300

rpc_int_t rpc_timer_init();

rpc_msec_t rpc_timer_find(void);

void rpc_timer_expire(void);

void rpc_timer_del(rpc_event_t *ev);

void rpc_timer_add(rpc_event_t *ev,rpc_msec_t timer);

extern rpc_rbtree_t rpc_timer_rbtree;

extern rpc_msec_t rpc_current_msec;

#endif
