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
  10,226,3,10,10,109,114,112,99,46,112,114,111,116,111,34,
  52,10,22,109,114,112,99,95,114,101,113,117,101,115,116,95,
  101,120,116,101,110,115,105,111,110,18,10,10,2,105,100,24,
  1,32,2,40,5,18,14,10,6,108,101,110,103,116,104,24,
  2,32,2,40,5,34,248,1,10,19,109,114,112,99,95,114,
  101,113,117,101,115,116,95,104,101,97,100,101,114,18,15,10,
  7,102,114,111,109,95,105,100,24,1,32,2,40,5,18,13,
  10,5,116,111,95,105,100,24,2,32,2,40,5,18,16,10,
  8,115,101,113,117,101,110,99,101,24,3,32,2,40,5,18,
  19,10,11,98,111,100,121,95,108,101,110,103,116,104,24,4,
  32,2,40,5,18,11,10,3,111,112,116,24,5,32,1,40,
  5,18,19,10,11,99,111,110,116,101,120,116,95,117,114,105,
  24,6,32,1,40,9,18,21,10,13,102,114,111,109,95,99,
  111,109,112,117,116,101,114,24,7,32,1,40,9,18,20,10,
  12,102,114,111,109,95,115,101,114,118,105,99,101,24,8,32,
  1,40,9,18,18,10,10,116,111,95,115,101,114,118,105,99,
  101,24,9,32,1,40,9,18,17,10,9,116,111,95,109,101,
  116,104,111,100,24,10,32,1,40,9,18,36,10,3,101,120,
  116,24,11,32,3,40,11,50,23,46,109,114,112,99,95,114,
  101,113,117,101,115,116,95,101,120,116,101,110,115,105,111,110,
  34,129,1,10,20,109,114,112,99,95,114,101,115,112,111,110,
  115,101,95,104,101,97,100,101,114,18,16,10,8,115,101,113,
  117,101,110,99,101,24,1,32,2,40,5,18,21,10,13,114,
  101,115,112,111,110,115,101,95,99,111,100,101,24,2,32,2,
  40,5,18,19,10,11,98,111,100,121,95,108,101,110,103,116,
  104,24,3,32,2,40,5,18,11,10,3,111,112,116,24,4,
  32,1,40,5,18,13,10,5,116,111,95,105,100,24,5,32,
  1,40,5,18,15,10,7,102,114,111,109,95,105,100,24,6,
  32,1,40,5,34,31,10,13,109,114,112,99,95,101,120,99,
  101,116,105,111,110,18,14,10,6,114,101,97,115,111,110,24,
  1,32,1,40,9,
};

int32_t
mrpc_args_init()
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

  rpc_pat_mrpc_request_header = pbc_pattern_new(env, "mrpc_request_header",    
      " from_id %d to_id %d sequence %d body_length %d opt %d context_uri %s from_computer %s from_service %s to_service %s to_method %s ext %a",
      offsetof(mrpc_request_header, from_id),
      offsetof(mrpc_request_header, to_id),
      offsetof(mrpc_request_header, sequence),
      offsetof(mrpc_request_header, body_length),
      offsetof(mrpc_request_header, opt),
      offsetof(mrpc_request_header, context_uri),
      offsetof(mrpc_request_header, from_computer),
      offsetof(mrpc_request_header, from_service),
      offsetof(mrpc_request_header, to_service),
      offsetof(mrpc_request_header, to_method),
      offsetof(mrpc_request_header, ext)
  );

  rpc_pat_mrpc_response_header = pbc_pattern_new(env, "mrpc_response_header",    
      " sequence %d response_code %d body_length %d opt %d to_id %d from_id %d",
      offsetof(mrpc_response_header, sequence),
      offsetof(mrpc_response_header, response_code),
      offsetof(mrpc_response_header, body_length),
      offsetof(mrpc_response_header, opt),
      offsetof(mrpc_response_header, to_id),
      offsetof(mrpc_response_header, from_id)
  );

  rpc_pat_mrpc_excetion = pbc_pattern_new(env, "mrpc_excetion",    
      " reason %s",
      offsetof(mrpc_excetion, reason)
  );

  rpc_pat_mrpc_request_extension = pbc_pattern_new(env, "mrpc_request_extension",    
      " id %d length %d",
      offsetof(mrpc_request_extension, id),
      offsetof(mrpc_request_extension, length)
  );

  init = 1;
  return 0;
}

