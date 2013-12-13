#ifndef __MRPC_MSP_H__
#define __MRPC_MSP_H__

typedef struct mrpc_req_buf_s {
      mrpc_connection_t *c;
      mrpc_message_t msg;
      int userid;
      int sequence;
      int cmd;
      int format;
      int compress;
      time_t time;
      int conn_status;
}mrpc_req_buf_t;

int mrpc_process_client_req(mrpc_connection_t *c);
int send_svr_response(mrpc_req_buf_t *svr_req_data, rec_msg_t *client_msg);
#endif
