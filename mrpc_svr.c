#include "proxy.h"
#include "mrpc_msp.h"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;
//static char* PROTOCOL = "MCP/3.0";

//mcp_appbean_proto.protocol = "mcp3.0"


static void mrpc_svr_recv(ev_t *ev,ev_file_item_t *fi)
{
        UNUSED(ev);
        int n;
        mrpc_connection_t *c = fi->data;
        D("rpc server recv"); 
        n = mrpc_recv(c);
        if (n == 0) {

                goto failed;
        }
        if ( n == -1) {
                E("recv return -1");
                return;
        }

        int r;
        for(;;) {
                r = mrpc_process_client_req(c);
                if (r < 0) {
                        W("Process client req error");
                        goto failed;
                } else if (r == 0) {
                        break;
                } else {
                        //
                }
        }

        return;

failed:
        W("failed, prepare close!");
        mrpc_conn_close(c);
        mrpc_conn_free(c);

        return;
}

static void mrpc_svr_send(ev_t *ev, ev_file_item_t *ffi)
{
        UNUSED(ev);
        D("mrpc_svr_send begins");
        mrpc_connection_t *c = ffi->data;
        if (mrpc_send(c) < 0) {
                W("mrpc_send failed, close connection!");
                goto failed;
        }
        return;

failed:
        mrpc_conn_close(c);
        mrpc_conn_free(c);
        return;
}

void mrpc_svr_accept(ev_t *ev, ev_file_item_t *ffi)
{
        UNUSED(ev);

        int f,err;
        mrpc_connection_t *c = NULL;

        //socklen_t sin_size = sizeof(master->addr);
        //f = accept(ffi->fd,&(master->addr),&sin_size);
        f = accept(ffi->fd,NULL,NULL);
        D("rpc server: fd#%d accepted",f);

        if (f > 0) {
                err = setnonblocking(f);
                if (err < 0) {
                        W("set nonblocking error");
                        close(f);
                        return;
                }
                c = mrpc_conn_new(NULL);
                if (!c) {
                        W("cannot malloc new mrpc_connetion");
                        close(f);
                        return;
                }

                c->fd = f;
                c->conn_status = MRPC_CONN_CONNECTED;
                int r = ev_add_file_item(worker->ev,
                                f,
                                EV_WRITABLE | EV_READABLE | EPOLLET,
                                c,
                                mrpc_svr_recv,
                                mrpc_svr_send);

                if (r < 0) {
                        E("add file item failed");
                        goto failed;
                }
        } else {
                E("accept error %s", strerror(errno));
        }

        return;

failed:
        if (c) {
                mrpc_conn_close(c);
                mrpc_conn_free(c);
        }
        return;
}
