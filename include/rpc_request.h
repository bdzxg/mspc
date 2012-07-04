/*
 * rpc_request.h
 *
 *  Created on: 2012-2-21
 *      Author: yanghu
 */

#ifndef RPC_REQUEST_H_
#define RPC_REQUEST_H_

#include "rpc_types.h"
#include "rpc_message.h"

BEGIN_DECLS

#define RPC_MASK_REQUEST  0x0BADBEE0

/**
send status may cause iov data free core dump
request_status
  ready     -- in queue
  send      -- out queue to send
  timeout   -- timeout
**/

typedef enum rpc_request_status_s {
  rpc_rs_ready,
  rpc_rs_send,
  rpc_rs_timeout
}rpc_request_status_t;

//TODO union for save space 
struct rpc_request_s {
  __RPC_MESSAGE_COMMON
 
  rpc_request_head_t  head;

  /* server filed */
  char *key;

  /* client field */
  void *data;
  rpc_callback_ptr cb;

  rpc_connection_t *connection;
  struct ev_loop *loop;
  struct ev_timer timer;
};

void rpc_request_free(rpc_request_t *req);

void rpc_request_fail(rpc_request_t *req, rpc_int_t code);

void rpc_request_set_timeout(rpc_request_t *req, struct ev_loop *l);

rpc_int_t rpc_request_process(rpc_connection_t *c);

void rpc_request_pb(rpc_connection_t *c, rpc_request_t *req, int seq);

rpc_request_t *rpc_request_create(char *service_name, char *method_name, 
    void *buf, size_t buf_len, rpc_callback_ptr cb, void *data, rpc_msec_t timeout);

/*
#define rpc_request_free(r)  do {  \
  if (r != NULL) { \
    rpc_request_header__free_unpacked(r->head, NULL); \
  } \
  rpc_free(r); \
} while(0) 
*/

END_DECLS
#endif /* RPC_REQUEST_H_ */
