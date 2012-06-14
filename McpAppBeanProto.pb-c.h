/* Generated by the protocol buffer compiler.  DO NOT EDIT! */

#ifndef PROTOBUF_C_McpAppBeanProto_2eproto__INCLUDED
#define PROTOBUF_C_McpAppBeanProto_2eproto__INCLUDED

#include <google/protobuf-c/protobuf-c.h>

PROTOBUF_C_BEGIN_DECLS


typedef struct _McpAppBeanProto McpAppBeanProto;
typedef struct _ReceiveResult ReceiveResult;


/* --- enums --- */


/* --- messages --- */

struct  _McpAppBeanProto
{
  ProtobufCMessage base;
  char *protocol;
  int32_t cmd;
  char *compacturi;
  int32_t userid;
  int32_t sequence;
  int32_t opt;
  ProtobufCBinaryData usercontext;
  ProtobufCBinaryData content;
  char *epid;
  int32_t zipflag;
};
#define MCP_APP_BEAN_PROTO__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&mcp_app_bean_proto__descriptor) \
    , NULL, 0, NULL, 0, 0, 0, {0,NULL}, {0,NULL}, NULL, 0 }


struct  _ReceiveResult
{
  ProtobufCMessage base;
  char *protocol;
};
#define RECEIVE_RESULT__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&receive_result__descriptor) \
    , NULL }


/* McpAppBeanProto methods */
void   mcp_app_bean_proto__init
                     (McpAppBeanProto         *message);
size_t mcp_app_bean_proto__get_packed_size
                     (const McpAppBeanProto   *message);
size_t mcp_app_bean_proto__pack
                     (const McpAppBeanProto   *message,
                      uint8_t             *out);
size_t mcp_app_bean_proto__pack_to_buffer
                     (const McpAppBeanProto   *message,
                      ProtobufCBuffer     *buffer);
McpAppBeanProto *
       mcp_app_bean_proto__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   mcp_app_bean_proto__free_unpacked
                     (McpAppBeanProto *message,
                      ProtobufCAllocator *allocator);
/* ReceiveResult methods */
void   receive_result__init
                     (ReceiveResult         *message);
size_t receive_result__get_packed_size
                     (const ReceiveResult   *message);
size_t receive_result__pack
                     (const ReceiveResult   *message,
                      uint8_t             *out);
size_t receive_result__pack_to_buffer
                     (const ReceiveResult   *message,
                      ProtobufCBuffer     *buffer);
ReceiveResult *
       receive_result__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   receive_result__free_unpacked
                     (ReceiveResult *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*McpAppBeanProto_Closure)
                 (const McpAppBeanProto *message,
                  void *closure_data);
typedef void (*ReceiveResult_Closure)
                 (const ReceiveResult *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor mcp_app_bean_proto__descriptor;
extern const ProtobufCMessageDescriptor receive_result__descriptor;

PROTOBUF_C_END_DECLS


#endif  /* PROTOBUF_McpAppBeanProto_2eproto__INCLUDED */