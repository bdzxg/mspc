/*
 * rpc_args.h
 *
 *  Created on: 2012-10-30
 *      Author: liulijin
 */

#ifndef _MSP_H_INCLUDED_
#define _MSP_H_INCLUDED_

#include "proxy.h"

typedef struct {
  int32_t from_id;
  int32_t to_id;
  int32_t sequence;
  int32_t body_length;
  int32_t opt;
  struct pbc_slice context_uri;
  struct pbc_slice from_computer;
  struct pbc_slice from_service;
  struct pbc_slice to_service;
  struct pbc_slice to_method;
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
  struct pbc_slice response;
  int32_t user_id;
  struct pbc_slice epid;
}reg3_args;

typedef struct {
  struct pbc_slice protocol;
  int32_t cmd;
  struct pbc_slice compact_uri;
  int32_t user_id;
  int32_t sequence;
  int32_t opt;
  struct pbc_slice user_context;
  struct pbc_slice content;
  struct pbc_slice epid;
  int32_t zip_flag;
}mcp_appbean_proto;

typedef struct {
  int32_t sid;
  struct pbc_slice mobile_no;
  struct pbc_slice email;
  struct pbc_slice user_type_str;
  struct pbc_slice language;
  struct pbc_slice nick_name;
  struct pbc_slice region;
  struct pbc_slice last_logoff_time;
  int32_t user_id;
  int32_t logical_pool;
  struct pbc_slice epid;
  struct pbc_slice client_ip;
  struct pbc_slice client_version;
  struct pbc_slice client_platform;
  struct pbc_slice machine_code;
  int32_t apn_type;
}user_context;

typedef struct {
  struct pbc_slice option;
}retval;

typedef struct {
  struct pbc_slice logoff_time;
  struct pbc_slice login_time;
  int32_t owner_id;
  int32_t owner_sid;
  int32_t client_type;
  struct pbc_slice client_version;
  int32_t apn;
  int64_t upflow;
  int64_t downflow;
  struct pbc_slice owner_region;
  struct pbc_slice owner_carrier;
  struct pbc_slice ip_address;
}mobile_flow_event_args;

typedef struct {
  int32_t id;
  int32_t length;
}mrpc_request_extension;


int rpc_args_init();

struct pbc_pattern *rpc_pat_mrpc_request_header;
struct pbc_pattern *rpc_pat_mrpc_response_header;
struct pbc_pattern *rpc_pat_reg3_args;
struct pbc_pattern *rpc_pat_mcp_appbean_proto;
struct pbc_pattern *rpc_pat_user_context;
struct pbc_pattern *rpc_pat_retval;
struct pbc_pattern *rpc_pat_mobile_flow_event_args;
struct pbc_pattern *rpc_pat_mrpc_request_extension;

#endif

