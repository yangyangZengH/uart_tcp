
#include "common.h"
#include "server_mod.h"


int main(int argc, char **argv)
{
	int ret = 0;
	st_tcp_server server;

	ret = tcp_server_init(&server);
	if (-1 == ret) {
		dbg_perror("tcp_server_init error.");
		return -1;
	}
	ret = tcp_server_start(&server);
	if (-1 == ret) {
		dbg_perror("tcp_server_start error.");
		return -1;
	}

	while (server.server_recv_thd_flag && server.server_keep_alive_thd_flag \
			&& server.server_listen_connect_thd_flag) {
		usleep(1000000);
	}

	ret = tcp_server_stop(&server);
	if (-1 == ret) {
		dbg_perror("tcp_server_stop error.");
		return -1;
	}
	ret = tcp_server_deinit(&server);
	if (-1 == ret) {
		dbg_perror("tcp_server_deinit error.");
		return -1;
	}

	return 0;
}
