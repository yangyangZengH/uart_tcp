
#include "common.h"
#include "app.h"
#include "fifo.h"
#include "uart_dev.h"


int main(int argc, char **argv)
{
	int ret = 0;
	st_manage_thd manage_thd;
	
	memset(&manage_thd, 0, sizeof(manage_thd));
	ret = app_init(&manage_thd);
	if (-1 == ret) {
		dbg_perror("app_init failed. ");
		return -1;
	}

	ret = app_start(&manage_thd);
	if (-1 == ret) {
		dbg_perror("app_init failed. ");
		return -1;
	}
	
	while (manage_thd.uart_dev.uart_fd != -1) {
		usleep(50000);
	}
	
	ret = app_stop(&manage_thd);
	if (-1 == ret) {
		dbg_perror("app_init failed. ");
		return -1;
	}

	ret = app_deinit(&manage_thd);
	if (-1 == ret) {
		dbg_perror("app_init failed. ");
		return -1;
	}

	return 0;
}
