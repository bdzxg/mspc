/*
 * 
 *
 *  Created on: 2012-09-21
 *      Author: likai 
 */

#include "rpc_args.h"

static rpc_pb_env *env = NULL;

static const char rpc_pb_desc[] =
{
  10,179,7,10,21,77,99,112,65,112,112,66,101,97,110,80,
  114,111,116,111,46,112,114,111,116,111,34,63,10,13,82,101,
  103,51,86,53,82,101,113,65,114,103,115,18,16,10,8,114,
  101,115,112,111,110,115,101,24,1,32,1,40,9,18,14,10,
  6,117,115,101,114,73,100,24,2,32,1,40,5,18,12,10,
  4,101,112,105,100,24,3,32,1,40,9,34,184,1,10,15,
  77,99,112,65,112,112,66,101,97,110,80,114,111,116,111,18,
  16,10,8,80,114,111,116,111,99,111,108,24,1,32,1,40,
  9,18,11,10,3,67,109,100,24,2,32,1,40,5,18,18,
  10,10,67,111,109,112,97,99,116,85,114,105,24,3,32,1,
  40,9,18,14,10,6,85,115,101,114,73,100,24,4,32,1,
  40,5,18,16,10,8,83,101,113,117,101,110,99,101,24,5,
  32,1,40,5,18,11,10,3,79,112,116,24,6,32,1,40,
  5,18,19,10,11,85,115,101,114,67,111,110,116,101,120,116,
  24,7,32,1,40,9,18,15,10,7,67,111,110,116,101,110,
  116,24,8,32,1,40,9,18,12,10,4,69,112,105,100,24,
  9,32,1,40,9,18,15,10,7,90,105,112,70,108,97,103,
  24,10,32,1,40,5,34,203,2,10,11,85,115,101,114,67,
  111,110,116,101,120,116,18,11,10,3,83,105,100,24,1,32,
  1,40,5,18,16,10,8,77,111,98,105,108,101,78,111,24,
  2,32,1,40,9,18,13,10,5,69,109,97,105,108,24,3,
  32,1,40,9,18,19,10,11,85,115,101,114,84,121,112,101,
  83,116,114,24,4,32,1,40,9,18,16,10,8,76,97,110,
  103,117,97,103,101,24,5,32,1,40,9,18,16,10,8,78,
  105,99,107,110,97,109,101,24,6,32,1,40,9,18,14,10,
  6,82,101,103,105,111,110,24,7,32,1,40,9,18,22,10,
  14,76,97,115,116,76,111,103,111,102,102,84,105,109,101,24,
  8,32,1,40,9,18,14,10,6,85,115,101,114,73,100,24,
  9,32,1,40,5,18,19,10,11,76,111,103,105,99,97,108,
  80,111,111,108,24,10,32,1,40,5,18,12,10,4,69,112,
  105,100,24,11,32,1,40,9,18,16,10,8,67,108,105,101,
  110,116,73,112,24,12,32,1,40,9,18,21,10,13,67,108,
  105,101,110,116,86,101,114,115,105,111,110,24,13,32,1,40,
  9,18,18,10,10,67,108,105,101,110,116,67,97,112,115,24,
  14,32,1,40,9,18,23,10,15,67,108,105,101,110,116,80,
  108,97,116,101,102,111,114,109,24,15,32,1,40,9,18,19,
  10,11,77,97,99,104,105,110,101,67,111,100,101,24,16,32,
  1,40,9,18,15,10,7,65,112,110,84,121,112,101,24,17,
  32,1,40,5,34,24,10,6,114,101,116,118,97,108,18,14,
  10,6,111,112,116,105,111,110,24,1,32,1,40,9,34,247,
  1,10,19,77,111,98,105,108,101,70,108,111,119,69,118,101,
  110,116,65,114,103,115,18,18,10,10,76,111,103,111,102,102,
  84,105,109,101,24,1,32,1,40,9,18,17,10,9,76,111,
  103,105,110,84,105,109,101,24,2,32,1,40,9,18,15,10,
  7,79,119,110,101,114,73,100,24,3,32,1,40,5,18,16,
  10,8,79,119,110,101,114,83,105,100,24,4,32,1,40,5,
  18,18,10,10,67,108,105,101,110,116,84,121,112,101,24,5,
  32,1,40,5,18,21,10,13,67,108,105,101,110,116,86,101,
  114,115,105,111,110,24,6,32,1,40,9,18,11,10,3,65,
  112,110,24,7,32,1,40,5,18,14,10,6,85,112,70,108,
  111,119,24,8,32,1,40,3,18,16,10,8,68,111,119,110,
  70,108,111,119,24,9,32,1,40,3,18,19,10,11,79,119,
  110,101,114,82,101,103,105,111,110,24,10,32,1,40,9,18,
  20,10,12,79,119,110,101,114,67,97,114,114,105,101,114,24,
  11,32,1,40,9,18,17,10,9,73,112,65,100,100,114,101,
  115,115,24,12,32,1,40,9,50,60,10,13,77,83,80,82,
  112,99,83,101,114,118,105,99,101,18,43,10,14,82,101,99,
  101,105,118,101,77,101,115,115,97,103,101,18,16,46,77,99,
  112,65,112,112,66,101,97,110,80,114,111,116,111,26,7,46,
  114,101,116,118,97,108,
};

rpc_int_t
rpc_args_init()
{
  static rpc_int_t init = 0;
  
  if (init) {
    return RPC_OK;
  }

  rpc_pb_string desc;

  desc.buffer = (void *)rpc_pb_desc;
  desc.len = sizeof(rpc_pb_desc);

  env = rpc_pb_env_new();
  rpc_pb_env_regdesc(env, &desc);

  rpc_pat_reg3v5reqargs = rpc_pb_pattern_new(env, "Reg3V5ReqArgs",    
      " response %s userId %d epid %s",
      offsetof(Reg3V5ReqArgs, response),
      offsetof(Reg3V5ReqArgs, userId),
      offsetof(Reg3V5ReqArgs, epid)
  );

  rpc_pat_mcpappbeanproto = rpc_pb_pattern_new(env, "McpAppBeanProto",    
      " Protocol %s Cmd %d CompactUri %s UserId %d Sequence %d Opt %d UserContext %s Content %s Epid %s ZipFlag %d",
      offsetof(McpAppBeanProto, Protocol),
      offsetof(McpAppBeanProto, Cmd),
      offsetof(McpAppBeanProto, CompactUri),
      offsetof(McpAppBeanProto, UserId),
      offsetof(McpAppBeanProto, Sequence),
      offsetof(McpAppBeanProto, Opt),
      offsetof(McpAppBeanProto, UserContext),
      offsetof(McpAppBeanProto, Content),
      offsetof(McpAppBeanProto, Epid),
      offsetof(McpAppBeanProto, ZipFlag)
  );

  rpc_pat_usercontext = rpc_pb_pattern_new(env, "UserContext",    
      " Sid %d MobileNo %s Email %s UserTypeStr %s Language %s Nickname %s Region %s LastLogoffTime %s UserId %d LogicalPool %d Epid %s ClientIp %s ClientVersion %s ClientPlateform %s MachineCode %s ApnType %d",
      offsetof(UserContext, Sid),
      offsetof(UserContext, MobileNo),
      offsetof(UserContext, Email),
      offsetof(UserContext, UserTypeStr),
      offsetof(UserContext, Language),
      offsetof(UserContext, Nickname),
      offsetof(UserContext, Region),
      offsetof(UserContext, LastLogoffTime),
      offsetof(UserContext, UserId),
      offsetof(UserContext, LogicalPool),
      offsetof(UserContext, Epid),
      offsetof(UserContext, ClientIp),
      offsetof(UserContext, ClientVersion),
      offsetof(UserContext, ClientPlateform),
      offsetof(UserContext, MachineCode),
      offsetof(UserContext, ApnType)
  );

  rpc_pat_retval = rpc_pb_pattern_new(env, "retval",    
      " option %s",
      offsetof(retval, option)
  );

  rpc_pat_mobilefloweventargs = rpc_pb_pattern_new(env, "MobileFlowEventArgs",    
      " LogoffTime %s LoginTime %s OwnerId %d OwnerSid %d ClientType %d ClientVersion %s Apn %d UpFlow %D DownFlow %D OwnerRegion %s OwnerCarrier %s IpAddress %s",
      offsetof(MobileFlowEventArgs, LogoffTime),
      offsetof(MobileFlowEventArgs, LoginTime),
      offsetof(MobileFlowEventArgs, OwnerId),
      offsetof(MobileFlowEventArgs, OwnerSid),
      offsetof(MobileFlowEventArgs, ClientType),
      offsetof(MobileFlowEventArgs, ClientVersion),
      offsetof(MobileFlowEventArgs, Apn),
      offsetof(MobileFlowEventArgs, UpFlow),
      offsetof(MobileFlowEventArgs, DownFlow),
      offsetof(MobileFlowEventArgs, OwnerRegion),
      offsetof(MobileFlowEventArgs, OwnerCarrier),
      offsetof(MobileFlowEventArgs, IpAddress)
  );

  init = 1;
  return RPC_OK;
}

