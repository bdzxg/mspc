#include "rpc_server.h"
#include "rpc_args.h"

/*
 * service name: SmsqService
 * method name: ReceiveMessage
 * input type: McpAppBeanProto
 * output type: retval
 */
void 
smsqservice_receivemessage(rpc_connection_t *c,void *buf, size_t buf_size)
{
  int r;
  //deserialize
  McpAppBeanProto input;
  rpc_pb_string str = {buf, buf_size};

  r = rpc_pb_pattern_unpack(rpc_pat_mcpappbeanproto, &str, &input);

  if ( r < 0) {
    rpc_return_error(c, RPC_CODE_INVALID_REQUEST_ARGS, "input decode failed!");
    return;
  }
  retval output;
  rpc_pb_string str_out;
  int outsz = 4096; //you can emphasize alloc mem size, also can alloc big 
  /*
     oops...
     how to implement?
  */

  str_out.buffer = rpc_connection_alloc(c, outsz);
  str_out.len = outsz;

  r = rpc_pb_pattern_pack(rpc_pat_retval, &output, &str_out);

  if (r >= 0) {
    rpc_return(c, str_out.buffer, str_out.len);
  } else {
    rpc_return_error(c, RPC_CODE_SERVER_ERROR, "output encode failed!");
  }
}


int 
main(int argc, char **argv)
{
  rpc_server_t *svr = rpc_server_new();

  //to implement
  /*
     oops...
     how to implement?

     rpc_server_regchannel(svr,"");
  */

  rpc_server_regservice(svr, "SmsqService", "ReceiveMessage", smsqservice_receivemessage);

  rpc_args_init();

  if (rpc_server_start(svr) != RPC_OK) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

