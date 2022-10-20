
#include "common.h"
#include "app.h"
#include "fifo.h"



int main(int argc, char **argv)
{
	int ret = 0;
	st_app app;

	ret = app_init(&app);
	if (-1 == ret) {
		dbg_perror("app_init error.");
		return -1;
	}

	ret = app_start(&app);
	if (-1 == ret) {
		dbg_perror("app_start error.");
		return -1;
	}

	while (-1 != app.uart_dev.uart_fd && (app.app_recv_thd_flag && \
			app.app_handle_thd_flag && app.app_send_thd_flag)) {
		usleep(500000);
	}

	ret = app_stop(&app);
	if (-1 == ret) {
		dbg_perror("app_stop error.");
		return -1;
	}
	ret = app_deinit(&app);
	if (-1 == ret) {
		dbg_perror("app_deinit error.");
		return -1;
	}

	return 0;
}
