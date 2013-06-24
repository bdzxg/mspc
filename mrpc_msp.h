#ifndef __MRPC_MSP_H__
#define __MRPC_MSP_H__

typedef struct mrpc_req_buf_s {
      mrpc_connection_t *c;
      mrpc_message_t msg;
      int sequence;
      time_t time;
}mrpc_req_buf_t;

int mrpc_process_client_req(mrpc_connection_t *c);
#endif
