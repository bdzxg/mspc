/*
 * rpc_args.h
 *
 *  Created on: 2012-08-23
 *      Author: likai
 */

#ifndef _MCPAPPBEANPROTO_H_INCLUDED_
#define _MCPAPPBEANPROTO_H_INCLUDED_

#include "rpc_types.h"


typedef struct {
  rpc_pb_string response;
  int32_t userId;
  rpc_pb_string epid;
}Reg3V5ReqArgs;

typedef struct {
  rpc_pb_string Protocol;
  int32_t Cmd;
  rpc_pb_string CompactUri;
  int32_t UserId;
  int32_t Sequence;
  int32_t Opt;
  rpc_pb_string UserContext;
  rpc_pb_string Content;
  rpc_pb_string Epid;
  int32_t ZipFlag;
}McpAppBeanProto;

typedef struct {
  int32_t Sid;
  rpc_pb_string MobileNo;
  rpc_pb_string Email;
  rpc_pb_string UserTypeStr;
  rpc_pb_string Language;
  rpc_pb_string Nickname;
  rpc_pb_string Region;
  rpc_pb_string LastLogoffTime;
  int32_t UserId;
  int32_t LogicalPool;
  rpc_pb_string Epid;
  rpc_pb_string ClientIp;
  rpc_pb_string ClientVersion;
  rpc_pb_string ClientPlateform;
  rpc_pb_string MachineCode;
  int32_t ApnType;
}UserContext;

typedef struct {
  rpc_pb_string option;
}retval;


rpc_int_t rpc_args_init();

rpc_pb_pattern *rpc_pat_reg3v5reqargs;
rpc_pb_pattern *rpc_pat_mcpappbeanproto;
rpc_pb_pattern *rpc_pat_usercontext;
rpc_pb_pattern *rpc_pat_retval;

#endif

