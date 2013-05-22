/*
 * rpc_args.h
 *
 *  Created on: 2012-12-20
 *      Author: liulijin
 */

#ifndef _MSP_PB_H_
#define _MSP_PB_H_

#include "proxy.h"

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
  struct pbc_slice ip;
  struct pbc_slice category_name;
}mcp_appbean_proto;

typedef struct {
  int32_t sid;
  struct pbc_slice mobile_no;
  struct pbc_slice email;
  struct pbc_slice user_type_str;
  struct pbc_slice language;
  struct pbc_slice nick_name;
  struct pbc_slice region;
  uint64_t last_logoff_time;
  int32_t user_id;
  int32_t logical_pool;
  struct pbc_slice epid;
  struct pbc_slice client_ip;
  struct pbc_slice client_version;
  struct pbc_slice client_caps;
  struct pbc_slice client_platform;
  struct pbc_slice machine_code;
  struct pbc_slice oemtag;
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

int32_t msp_args_init();

struct pbc_pattern *rpc_pat_reg3_args;
struct pbc_pattern *rpc_pat_mcp_appbean_proto;
struct pbc_pattern *rpc_pat_user_context;
struct pbc_pattern *rpc_pat_retval;
struct pbc_pattern *rpc_pat_mobile_flow_event_args;
struct pbc_pattern *rpc_pat_mrpc_request_extension;

#endif
