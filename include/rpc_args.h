/*
 * rpc_args.h
 *
 *  Created on: 2012-06-29
 *      Author: likai
 */

#ifndef _MCPAPPBEANPROTO_H_INCLUDED_
#define _MCPAPPBEANPROTO_H_INCLUDED_

#include "rpc_types.h"


typedef struct {
  rpc_pb_string option;
}retval;

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


rpc_int_t rpc_args_init();

rpc_pb_pattern *rpc_pat_retval;
rpc_pb_pattern *rpc_pat_mcpappbeanproto;

#endif

