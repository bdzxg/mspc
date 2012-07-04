
/*
 *  Copyright (C) tiger yang
 */

#ifndef _RPC_MESSAGE_H_INCLUDED_
#define _RPC_MESSAGE_H_INCLUDED_ 

#include "rpc_types.h"

BEGIN_DECLS

typedef struct rpc_packet_head_s rpc_packet_head_t;

#define __RPC_MESSAGE_COMMON \
  rpc_parse_state st; \
  rpc_packet_head_t *packet_head; \
  size_t packet_length; \
  uint8_t *message_head; \
  size_t message_head_length; \
  void *body; \
  size_t  body_length; 

#define RPC_PACKET_HEAD_LEN sizeof(rpc_packet_head_t)
#define RPC_REQ_MAX_HEAD_LEN 114
#define RPC_RSP_MAX_HEAD_LEN 24


typedef enum {
  rpc_ps_none =0, //包未开始解析
  rpc_ps_pheader_done, //包头已解析
  rpc_ps_header_done, //头已解析
  rpc_ps_done //包解析完毕
}rpc_parse_state;

struct rpc_packet_head_s {
  uint32_t  packet_mask;
  uint32_t  packet_length;
  uint16_t  header_length;
  uint16_t  packet_options;
};

typedef struct {
  int32_t from_id;
  int32_t to_id;
  int32_t sequence;
  int32_t body_length;
  int32_t option;
  rpc_pb_string context_uri;
  rpc_pb_string from_computer;
  rpc_pb_string from_service;
  rpc_pb_string to_service;
  rpc_pb_string to_method;
  rpc_pb_array  extensions;
}rpc_request_head_t;

typedef struct {
  int32_t id;
  int32_t length;
}rpc_request_exension_t;

typedef struct {
  int32_t sequence;
  int32_t response_code;
  int32_t body_length;
  int32_t option;
  int32_t to_id;
  int32_t from_id;
}rpc_response_head_t;

rpc_int_t rpc_packet_rev(rpc_connection_t *c);

rpc_int_t rpc_pb_init();

extern rpc_pb_pattern *rpc_pat_head_req;
extern rpc_pb_pattern *rpc_pat_req_ex;
extern rpc_pb_pattern *rpc_pat_head_rsp;

END_DECLS

#endif

