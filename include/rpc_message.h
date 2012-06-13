
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_MESSAGE_H_INCLUDED_
#define _RPC_MESSAGE_H_INCLUDED_ 

#include "rpc_types.h"

#define RPC_MASK_REQUEST  0x0BADBEE0
#define RPC_MASK_RESPONSE 0x0BADBEE1
#define RPC_PACKET_HEADER_LEN sizeof(rpc_packet_head_t)

typedef struct rpc_packet_head_s rpc_packet_head_t;

struct rpc_packet_head_s {
  uint32_t  packet_mask;
  uint32_t  packet_length;
  uint16_t  header_length;
  uint16_t  packet_options;
};

struct rpc_packet_s {
  rpc_pool_t *pool;
  rpc_packet_head_t *packet_head;
  size_t packet_length;
  uint8_t *message_head;
  size_t message_head_length;
  void *body;
  size_t body_length;

  void *header;
};
rpc_packet_t *rpc_packet_create();

void rpc_packet_free(rpc_packet_t *pkt);

rpc_packet_t *rpc_packet_create_req(char *service_name, char *method_name,void *buf,size_t buf_len );

rpc_packet_t *rpc_packet_create_rsp(rpc_packet_t *request, rpc_int_t rsp_code, void *buf, size_t buf_len);

rpc_int_t rpc_packet_req_handler(rpc_connection_t *c);

rpc_int_t rpc_packet_rsp_handler(rpc_connection_t *c, void **buf,size_t *buf_len);

rpc_int_t rpc_packet_receive(rpc_connection_t *c,rpc_msec_t timer, void **buf, size_t *buf_len);

#endif

