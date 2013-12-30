#include "proxy.h"
#include "mrpc.h"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;
extern pxy_settings setting;
static mrpc_upstreamer_t mrpc_up;
//static char* PROTOCOL = "MCP/3.0";


int mrpc_start()
{
	int fd = mrpc_up.listen_fd;
	if (listen(fd, 1024) < 0) {
		E("rpc listen error");
		return -1;
	}
	
	int r = ev_add_file_item(worker->ev, fd, EV_READABLE, 
				 NULL, mrpc_svr_accept, NULL);
	if (r < 0) {
		E("create ev for listen fd error");
		goto start_failed;
	}
	return 0;

start_failed:
	return -1;
}


int mrpc_init()
{
	D("mrpc init begins");
	mrpc_args_init();
        mrpc_init_stash_conns();

        struct sockaddr_in addr1;
	mrpc_up.listen_fd = -1;
	INIT_LIST_HEAD(&mrpc_up.conn_list);

	mrpc_up.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (mrpc_up.listen_fd < 0)  {
		E("create rpc listen fd error");
		return -1;
	}
	int reuse = 1;
	setsockopt(mrpc_up.listen_fd , SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	
	if (setnonblocking(mrpc_up.listen_fd) < 0) {
		E("set nonblocling error");
		return -1;
	}

	addr1.sin_family = AF_INET;
	addr1.sin_port = htons(setting.backend_port);
	addr1.sin_addr.s_addr = 0;
	
	if (bind(mrpc_up.listen_fd, (struct sockaddr*)&addr1, sizeof(addr1)) < 0) {
		E("bind error");
		return -1;
	}
	D("mrpc init finish, port %d", setting.backend_port);
	return 0;
}
