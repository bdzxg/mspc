/*
 * rpc_util.h
 *
 *  Created on: 2012-2-29
 *      Author: yanghu
 */

#ifndef RPC_UTIL_H_
#define RPC_UTIL_H_

#include "rpc_types.h"

BEGIN_DECLS

const char* rpc_code_format(const rpc_int_t code);

rpc_int_t rpc_str_code(const char* str);

void rpc_sleep(int ms);

END_DECLS

#endif /* RPC_UTIL_H_ */
