/*
 * rpc_args.h
 *
 *  Created on: 2012-10-30
 *      Author: liulijin
 */

#ifndef _MSP_H_INCLUDED_
#define _MSP_H_INCLUDED_

#inclide "proxy.h"

typedef struct {
  int32_t from_id;
  int32_t to_id;
  int32_t sequence;
  int32_t body_length;
  int32_t opt;
  pbc_slice context_uri;
  pbc_slice from_computer;
  pbc_slice from_service;
  pbc_slice to_service;
  pbc_slice to_method;
  pbc_array ext;
}mrpc_request_header;

typedef struct {
  int32_t sequence;
  int32_t response_code;
  int32_t body_length;
  int32_t opt;
  int32_t to_id;
  int32_t from_id;
}mrpc_response_header;

typedef struct {
  pbc_slice response;
  int32_t user_id;
  pbc_slice epid;
}reg3_args;

typedef struct {
  pbc_slice protocol;
  int32_t cmd;
  pbc_slice compact_uri;
  int32_t user_id;
  int32_t sequence;
  int32_t opt;
  pbc_slice user_context;
  pbc_slice content;
  pbc_slice epid;
  int32_t zip_flag;
}mcp_appbean_proto;

typedef struct {
  int32_t sid;
  pbc_slice mobile_no;
  pbc_slice email;
  pbc_slice user_type_str;
  pbc_slice language;
  pbc_slice nick_name;
  pbc_slice region;
  pbc_slice last_logoff_time;
  int32_t user_id;
  int32_t logical_pool;
  pbc_slice epid;
  pbc_slice client_ip;
  pbc_slice client_version;
  pbc_slice client_platform;
  pbc_slice machine_code;
  int32_t apn_type;
}user_context;

typedef struct {
  pbc_slice option;
}retval;

typedef struct {
  pbc_slice logoff_time;
  pbc_slice login_time;
  int32_t owner_id;
  int32_t owner_sid;
  int32_t client_type;
  pbc_slice client_version;
  int32_t apn;
  int64_t upflow;
  int64_t downflow;
  pbc_slice owner_region;
  pbc_slice owner_carrier;
  pbc_slice ip_address;
}mobile_flow_event_args;

typedef struct {
  int32_t id;
  int32_t length;
}mrpc_request_extension;


rpc_int_t rpc_args_init();

pbc_pattern *rpc_pat_mrpc_request_header;
pbc_pattern *rpc_pat_mrpc_response_header;
pbc_pattern *rpc_pat_reg3_args;
pbc_pattern *rpc_pat_mcp_appbean_proto;
pbc_pattern *rpc_pat_user_context;
pbc_pattern *rpc_pat_retval;
pbc_pattern *rpc_pat_mobile_flow_event_args;
pbc_pattern *rpc_pat_mrpc_request_extension;

#endif

