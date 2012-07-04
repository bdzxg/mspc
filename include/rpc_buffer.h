
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_BUFFER_H_INCLUDED_
#define _RPC_BUFFER_H_INCLUDED_

#include "rpc_types.h"

struct rpc_buf_s {
  u_char  *pos;
  u_char  *last;
  u_char  *start;
  u_char  *end;
  size_t size;

};

rpc_buf_t *rpc_buf_create(size_t size);

/* buf reset */
//rpc_int_t rpc_buf_reset(rpc_buf_t *b);
#define rpc_buf_reset(b) do {                         \
  b->pos = b->start;                                  \
  b->last = b->start;                                 \
} while(0)

#define rpc_buf_move(b)  do {                         \
  if (b->pos == b->last) {                            \
    rpc_buf_reset(b);                                 \
    break;                                            \
  }                                                   \
  if((b->pos != b->start) && (b->last > b->pos)) {    \
    rpc_log_debug("rpc: buf memmove");                     \
    rpc_memmove(b->start, b->pos, b->last - b->pos);  \
    b->last = b->start + (b->last - b->pos);          \
    b->pos = b->start;                                \
  }                                                   \
}while(0)

rpc_int_t rpc_buf_expand(rpc_buf_t *b);

#define rpc_buf_free(b) do {                          \
  rpc_free(b->start);                                 \
  rpc_free(b);                                        \
}while(0)
#endif
