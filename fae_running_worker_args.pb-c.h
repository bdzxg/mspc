/* Generated by the protocol buffer compiler.  DO NOT EDIT! */

#ifndef PROTOBUF_C_fae_5frunning_5fworker_5fargs_2eproto__INCLUDED
#define PROTOBUF_C_fae_5frunning_5fworker_5fargs_2eproto__INCLUDED

#include <google/protobuf-c/protobuf-c.h>

PROTOBUF_C_BEGIN_DECLS


typedef struct _FAERunningWorker FAERunningWorker;


/* --- enums --- */


/* --- messages --- */

struct  _FAERunningWorker
{
  ProtobufCMessage base;
  char *appworkerid;
  char *servername;
  char *serviceurls;
  uint64_t lasttime;
};
#define FAE__RUNNING_WORKER__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&fae__running_worker__descriptor) \
    , NULL, NULL, NULL, 0 }


/* FAERunningWorker methods */
void   fae__running_worker__init
                     (FAERunningWorker         *message);
size_t fae__running_worker__get_packed_size
                     (const FAERunningWorker   *message);
size_t fae__running_worker__pack
                     (const FAERunningWorker   *message,
                      uint8_t             *out);
size_t fae__running_worker__pack_to_buffer
                     (const FAERunningWorker   *message,
                      ProtobufCBuffer     *buffer);
FAERunningWorker *
       fae__running_worker__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   fae__running_worker__free_unpacked
                     (FAERunningWorker *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*FAERunningWorker_Closure)
                 (const FAERunningWorker *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor fae__running_worker__descriptor;

PROTOBUF_C_END_DECLS


#endif  /* PROTOBUF_fae_5frunning_5fworker_5fargs_2eproto__INCLUDED */