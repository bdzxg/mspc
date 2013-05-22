/*
 * 
 *
 *  Created on: 2012-12-20
 *      Author: liulijin 
 */

#include "proxy.h"

static struct pbc_env *env = NULL;

static const char rpc_pb_desc[] = 
{
	10,251,6,10,9,109,115,112,46,112,114,111,116,111,34,225,
	1,10,17,109,99,112,95,97,112,112,98,101,97,110,95,112,
	114,111,116,111,18,16,10,8,112,114,111,116,111,99,111,108,
	24,1,32,1,40,9,18,11,10,3,99,109,100,24,2,32,
	1,40,5,18,19,10,11,99,111,109,112,97,99,116,95,117,
	114,105,24,3,32,1,40,9,18,15,10,7,117,115,101,114,
	95,105,100,24,4,32,1,40,5,18,16,10,8,115,101,113,
	117,101,110,99,101,24,5,32,1,40,5,18,11,10,3,111,
	112,116,24,6,32,1,40,5,18,20,10,12,117,115,101,114,
	95,99,111,110,116,101,120,116,24,7,32,1,40,9,18,15,
	10,7,99,111,110,116,101,110,116,24,8,32,1,40,9,18,
	12,10,4,101,112,105,100,24,9,32,1,40,9,18,16,10,
	8,122,105,112,95,102,108,97,103,24,10,32,1,40,5,18,
	10,10,2,105,112,24,11,32,1,40,9,18,21,10,13,99,
	97,116,101,103,111,114,121,95,110,97,109,101,24,12,32,1,
	40,9,34,233,2,10,12,117,115,101,114,95,99,111,110,116,
	101,120,116,18,11,10,3,115,105,100,24,1,32,1,40,5,
	18,17,10,9,109,111,98,105,108,101,95,110,111,24,2,32,
	1,40,9,18,13,10,5,101,109,97,105,108,24,3,32,1,
	40,9,18,21,10,13,117,115,101,114,95,116,121,112,101,95,
	115,116,114,24,4,32,1,40,9,18,16,10,8,108,97,110,
	103,117,97,103,101,24,5,32,1,40,9,18,17,10,9,110,
	105,99,107,95,110,97,109,101,24,6,32,1,40,9,18,14,
	10,6,114,101,103,105,111,110,24,7,32,1,40,9,18,24,
	10,16,108,97,115,116,95,108,111,103,111,102,102,95,116,105,
	109,101,24,8,32,1,40,6,18,15,10,7,117,115,101,114,
	95,105,100,24,9,32,1,40,5,18,20,10,12,108,111,103,
	105,99,97,108,95,112,111,111,108,24,10,32,1,40,5,18,
	12,10,4,101,112,105,100,24,11,32,1,40,9,18,17,10,
	9,99,108,105,101,110,116,95,105,112,24,12,32,1,40,9,
	18,22,10,14,99,108,105,101,110,116,95,118,101,114,115,105,
	111,110,24,13,32,1,40,9,18,19,10,11,99,108,105,101,
	110,116,95,99,97,112,115,24,14,32,1,40,9,18,23,10,
	15,99,108,105,101,110,116,95,112,108,97,116,102,111,114,109,
	24,15,32,1,40,9,18,20,10,12,109,97,99,104,105,110,
	101,95,99,111,100,101,24,16,32,1,40,9,18,14,10,6,
	111,101,109,116,97,103,24,17,32,1,40,9,18,16,10,8,
	97,112,110,95,116,121,112,101,24,18,32,1,40,5,34,24,
	10,6,114,101,116,118,97,108,18,14,10,6,111,112,116,105,
	111,110,24,1,32,1,40,9,34,131,2,10,22,109,111,98,
	105,108,101,95,102,108,111,119,95,101,118,101,110,116,95,97,
	114,103,115,18,19,10,11,108,111,103,111,102,102,95,116,105,
	109,101,24,1,32,1,40,9,18,18,10,10,108,111,103,105,
	110,95,116,105,109,101,24,2,32,1,40,9,18,16,10,8,
	111,119,110,101,114,95,105,100,24,3,32,1,40,5,18,17,
	10,9,111,119,110,101,114,95,115,105,100,24,4,32,1,40,
	5,18,19,10,11,99,108,105,101,110,116,95,116,121,112,101,
	24,5,32,1,40,5,18,22,10,14,99,108,105,101,110,116,
	95,118,101,114,115,105,111,110,24,6,32,1,40,9,18,11,
	10,3,97,112,110,24,7,32,1,40,5,18,14,10,6,117,
	112,102,108,111,119,24,8,32,1,40,3,18,16,10,8,100,
	111,119,110,102,108,111,119,24,9,32,1,40,3,18,20,10,
	12,111,119,110,101,114,95,114,101,103,105,111,110,24,10,32,
	1,40,9,18,21,10,13,111,119,110,101,114,95,99,97,114,
	114,105,101,114,24,11,32,1,40,9,18,18,10,10,105,112,
	95,97,100,100,114,101,115,115,24,12,32,1,40,9
};

int32_t
msp_args_init()
{
  static int32_t init = 0;
  
  if (init) {
    return 0;
  }

  struct pbc_slice desc;

  desc.buffer = (void *)rpc_pb_desc;
  desc.len = sizeof(rpc_pb_desc);

  env = pbc_new();
  pbc_register(env, &desc);

  rpc_pat_user_context = pbc_pattern_new(env, "user_context",    
      " sid %d mobile_no %s email %s user_type_str %s language %s nick_name %s region %s last_logoff_time %D user_id %d logical_pool %d epid %s client_ip %s client_version %s client_caps %s client_platform %s machine_code %s oemtag %s apn_type %d",
      offsetof(user_context, sid),
      offsetof(user_context, mobile_no),
      offsetof(user_context, email),
      offsetof(user_context, user_type_str),
      offsetof(user_context, language),
      offsetof(user_context, nick_name),
      offsetof(user_context, region),
      offsetof(user_context, last_logoff_time),
      offsetof(user_context, user_id),
      offsetof(user_context, logical_pool),
      offsetof(user_context, epid),
      offsetof(user_context, client_ip),
      offsetof(user_context, client_version),
      offsetof(user_context, client_caps),
      offsetof(user_context, client_platform),
      offsetof(user_context, machine_code),
      offsetof(user_context, oemtag), 
      offsetof(user_context, apn_type)
  );

  rpc_pat_retval = pbc_pattern_new(env, "retval",    
      " option %s",
      offsetof(retval, option)
  );

  rpc_pat_mobile_flow_event_args = pbc_pattern_new(env, "mobile_flow_event_args",    
      " logoff_time %s login_time %s owner_id %d owner_sid %d client_type %d client_version %s apn %d upflow %D downflow %D owner_region %s owner_carrier %s ip_address %s",
      offsetof(mobile_flow_event_args, logoff_time),
      offsetof(mobile_flow_event_args, login_time),
      offsetof(mobile_flow_event_args, owner_id),
      offsetof(mobile_flow_event_args, owner_sid),
      offsetof(mobile_flow_event_args, client_type),
      offsetof(mobile_flow_event_args, client_version),
      offsetof(mobile_flow_event_args, apn),
      offsetof(mobile_flow_event_args, upflow),
      offsetof(mobile_flow_event_args, downflow),
      offsetof(mobile_flow_event_args, owner_region),
      offsetof(mobile_flow_event_args, owner_carrier),
      offsetof(mobile_flow_event_args, ip_address)
  );

  rpc_pat_mcp_appbean_proto = pbc_pattern_new(env, "mcp_appbean_proto",    
      " protocol %s cmd %d compact_uri %s user_id %d sequence %d opt %d user_context %s content %s epid %s zip_flag %d ip %s category_name %s",
      offsetof(mcp_appbean_proto, protocol),
      offsetof(mcp_appbean_proto, cmd),
      offsetof(mcp_appbean_proto, compact_uri),
      offsetof(mcp_appbean_proto, user_id),
      offsetof(mcp_appbean_proto, sequence),
      offsetof(mcp_appbean_proto, opt),
      offsetof(mcp_appbean_proto, user_context),
      offsetof(mcp_appbean_proto, content),
      offsetof(mcp_appbean_proto, epid),
      offsetof(mcp_appbean_proto, zip_flag),
      offsetof(mcp_appbean_proto, ip),
      offsetof(mcp_appbean_proto, category_name)
  );

  init = 1;
  return 0;
}
