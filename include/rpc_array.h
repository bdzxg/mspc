/*
 * rpc_array.h
 *
 *  Created on: 2011-3-24
 *      Author: yanghu
 */

#ifndef RPC_ARRAY_H_
#define RPC_ARRAY_H_

#include "rpc_types.h"

BEGIN_DECLS

struct rpc_array_s {
  pthread_mutex_t lock;
  void** data;
  rpc_uint_t size;
  rpc_uint_t capacity;
};

#define RPC_ARRAY_CAPACIRY 32

#define rpc_array_new() rpc_array_new_size(RPC_ARRAY_CAPACIRY)

#define rpc_array_lock(arr) pthread_mutex_lock(&arr->lock)

#define rpc_array_unlock(arr) pthread_mutex_unlock(&arr->lock)

rpc_array_t* rpc_array_new_size(int capacity);

#define rpc_array_index(arr, i) arr->data[i]

#define rpc_array_remove_all(arr) arr->size = 0

void rpc_array_remove_data(rpc_array_t *array, void *data);

void rpc_array_remove(rpc_array_t *array, int index);

void rpc_array_remove_unlock(rpc_array_t *array, int index);

void rpc_array_add(rpc_array_t *arr, void *data);

void rpc_array_add_unlock(rpc_array_t *arr, void *data);

void rpc_array_free(rpc_array_t *arr);

#define rpc_array_size(arr) arr->size

#define rpc_array_capacity(arr) arr->capacity

END_DECLS

#endif /* RPC_ARRAY_H_ */
