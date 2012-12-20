/*
 * rpc_args.h
 *
 *  Created on: 2012-12-20
 *      Author: liulijin
 */

#ifndef _MRPC_PB_H_INCLUDED_
#define _MRPC_PB_H_INCLUDED_

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
  struct pbc_slice reason;
}mrpc_excetion;

typedef struct {
  int32_t id;
  int32_t length;
}mrpc_request_extension;


int32_t mrpc_args_init();

struct pbc_pattern *rpc_pat_mrpc_request_header;
struct pbc_pattern *rpc_pat_mrpc_response_header;
struct pbc_pattern *rpc_pat_mrpc_excetion;
struct pbc_pattern *rpc_pat_mrpc_request_extension;

#endif

