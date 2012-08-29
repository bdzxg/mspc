#include "rpc_server.h"
#include "rpc_args.h"


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


  rpc_args_init();

  if (rpc_server_start(svr) != RPC_OK) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

