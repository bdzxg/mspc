/*
 * rpc_response.h
 *
 *  Created on: 2012-2-21
 *      Author: yanghu
 */

#ifndef RPC_RESPONSE_H_
#define RPC_RESPONSE_H_

#include "rpc_types.h"
#include "rpc_message.h"

#define RPC_MASK_RESPONSE 0x0BADBEE1

typedef struct rpc_response_s rpc_response_t;

struct rpc_response_s {
  __RPC_MESSAGE_COMMON

  rpc_response_head_t head;
};

rpc_response_t *rpc_response_create(rpc_connection_t *c, rpc_int_t rsp_code, void *buf, size_t buf_len);

rpc_int_t rpc_response_process(rpc_connection_t *c);

void rpc_response_free(rpc_response_t *rsp);

#endif /* RPC_RESPONSE_H_ */
