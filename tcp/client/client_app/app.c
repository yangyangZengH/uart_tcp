#include "app.h"
#include <sys/select.h> 




enum msg_cmd {
	HIP_USER_LOGIN          = 0x02,      //用户登录
    HIP_UART_PASS_THROUGH 	= 0x04,		 //串口透传
    HUP_UART_PRINT      	= 0x14,      //串口打印
	HUP_END_PROGRAM			= 0x15,		 //结束程序
};

/******************************************************************************************
 * Function Name :  uart_data_print
 * Description   :  串口打印，将数据hup封包，写入send_fifo(保留uart&&fifo功能)
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  无
*******************************************************************************************/
static void uart_data_print(st_app *app_ptr)
{
	int ret = 0;
	int length = 0;
	uint8_t temp_buf[255] = {0};
	uint8_t temp_len[2] = {0};

	if (NULL == app_ptr) 
        dbg_perror("uart_data_print arg error.");

	printf("---------------------------------\n");
	printf("-----------cmd对应的操作---------\n");	
	printf("---------------------------------\n");

	memset(temp_buf, 0, sizeof(temp_buf));
	ret = hup_pack(&app_ptr->hup_pack, temp_buf);
	if (-1 == ret) {
		dbg_perror("hup_pack failed!\n");
		return;
	}
	dbg_printf("hup_pack succeed .\n");

	memset(temp_len, 0, sizeof(temp_len));
	length = app_ptr->hup_pack.length;
	temp_len[0] = length >> 8;
	temp_len[1] = length & 0xff;					
	ret = fifo_write(&app_ptr->send_fifo, temp_len, 2);
	if (-1 == ret) {
		dbg_perror("fifo_write length failed!\n");				
		return;
	}		
	ret = fifo_write(&app_ptr->send_fifo, temp_buf, length);	
	if (-1 == ret) {
		dbg_perror("fifo_write failed!\n");				
		return;
	}
}

/******************************************************************************************
 * Function Name :  hip_user_login
 * Description   :  用户登录，初始化并启动客户端，使之与服务端建立连接
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  无
*******************************************************************************************/
static void hip_user_login(st_app *app_ptr)
{
	int ret = 0;

	if (NULL == app_ptr) {
        dbg_perror("hip_end_program arg error.");	
		return ;	
	}
	
	ret = tcp_client_init(&app_ptr->client);
	if (-1 == ret ) {
		dbg_perror("tcp_client_init error.");
		return;
	}
	ret = tcp_client_start(&app_ptr->client);
	if (-1 == ret ) {
		dbg_perror("tcp_client_start error.");
		return;
	}
}

/******************************************************************************************
 * Function Name :  hip_end_program
 * Description   :  程序结束，停止线程，关闭客户端释放资源
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  无
*******************************************************************************************/
static void hip_end_program(st_app *app_ptr)
{
	int ret = 0;

	if (NULL == app_ptr) {
        dbg_perror("hip_end_program arg error.");	
		return ;	
	}

	app_ptr->app_recv_thd_flag   = false;
	app_ptr->app_handle_thd_flag = false;
	app_ptr->app_send_thd_flag   = false;
	ret = tcp_client_stop(&app_ptr->client);
	if (-1 == ret ) {
		dbg_perror("tcp_client_stop error.");
		return;
	}
	ret = tcp_client_deinit(&app_ptr->client);
	if (-1 == ret ) {
		dbg_perror("tcp_client_deinit error.");
		return;
	}
}

/******************************************************************************************
 * Function Name :  depack_success_handle
 * Description   :  解包成功的后续操作，根据cmd命令类型处理不同逻辑
 * Parameters    :  app_ptr：st_app结构体指针
 * 					cmd：命令类型
 * 					uart_data：串口数据
 * 					length：串口数据长度
 * Returns       :  无
*******************************************************************************************/
static void depack_success_handle(st_app *app_ptr, uint8_t cmd, uint8_t *uart_data, uint16_t length)
{
	if (NULL == uart_data || NULL == app_ptr) {
        dbg_perror("depack_success_handle arg error.");		
	}

	switch (cmd) {
		case HIP_USER_LOGIN:
		printf(" HIP_USER_LOGIN : \n");
			hip_user_login(app_ptr);			
			break;
		case HIP_UART_PASS_THROUGH:
		printf(" HIP_UART_PASS_THROUGH : \n");
			client_mod_send(&app_ptr->client, cmd, length, uart_data);	
			break;
		case HUP_UART_PRINT:
			uart_data_print(app_ptr);
			break;
		case HUP_END_PROGRAM:
			hip_end_program(app_ptr);
			break;
		default:
			dbg_printf("depack_success_handle switch default. \n");
			break;
	}
}

/******************************************************************************************
 * Function Name :  uart_dev_isread
 * Description   :  监听串口是否可读
 * Parameters    :  fd：需要监听的文件描述符
  					sec：超时时间 单位 秒
   					usec：超时时间 单位 微妙
 * Returns       :  >0：表示被监视的文件描述符有变化。
					-1：表示select出错。
					 0：表示超时
*******************************************************************************************/
int uart_dev_isread(int fd, int sec, int usec)
{
	int ret = 0;
	struct timeval timeout;
	fd_set read_fd;

	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	FD_ZERO(&read_fd);
	FD_SET(fd,&read_fd);

	ret = select(fd + 1, &read_fd, NULL, NULL, &timeout);
	return ret;
}

/******************************************************************************************
 * Function Name :  app_recv_thread
 * Description   :  A、监听pc端发来的串口数据
 *					B、不解析，将数据转存到recv_fifo
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *app_recv_thread(void *arg)
{
	int ret = 0;
	int count = 0;
	uint8_t temp_buf[255] = {0};
	st_app *app_ptr = (st_app*)arg;

	if (NULL == app_ptr) {
        dbg_perror("app_ptr_start failed!");
		exit(-1);
    }


	while (app_ptr->app_recv_thd_flag) {
		if (uart_dev_isread(app_ptr->uart_dev.uart_fd, 5, 0) > 0) {
			memset(temp_buf, 0, sizeof(temp_buf));
			ret = uart_dev_read(app_ptr->uart_dev.uart_fd, temp_buf, sizeof(temp_buf));
			if (-1 == ret) {
				dbg_perror("uart_dev_read failed!\n");
				continue;
			}
 			/* 不解析，将读取到的数据直接weite()写进recv_fifo */
 			ret = fifo_write(&app_ptr->recv_fifo, temp_buf, count);
 			if (-1 == ret) {
				dbg_perror("fifo_write failed!\n");
				continue;
			}
			dbg_printf("write --%d-- FIFO .\n", ret);
 		}
	}
}

/******************************************************************************************
 * Function Name :  app_handle_thread
 * Description   :  A、读recv_fifo，逐个字节读取
 *					B、按照串口协议解析数据
 *					C、解析成功处理cmd id
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *app_handle_thread(void *arg)
{
	int ret = 0;
	uint16_t count = 0;
	uint8_t temp_buf[255] = {0};
	uint8_t temp_len[2] = {0};
	uint8_t pack_data = 0;
	st_app *app_ptr = (st_app*)arg;

	if (NULL == app_ptr) {
        dbg_perror("app_ptr_start failed!");
		exit(-1);
    }

	while (app_ptr->app_handle_thd_flag) {
		if (app_ptr->recv_fifo.avail_len > 0) {
			memset(temp_buf, 0, sizeof(temp_buf));
			app_ptr->hup_pack.depack_status = false;			
			/* 读FIFO , 解析hup */
			if (!app_ptr->hup_pack.depack_status) {
				if (!app_ptr->hup_pack.length_status) {
					ret = fifo_read_byte(&app_ptr->recv_fifo, &pack_data);
					if (-1 == ret) {
						dbg_perror("fifo_read failed!\n");				
						continue;
					}
					count++;
					hup_depack(pack_data, &app_ptr->hup_pack);
				} else {
					ret = fifo_read(&app_ptr->recv_fifo, temp_buf, app_ptr->hup_pack.length);
					if (-1 == ret) {
						dbg_perror("fifo_read failed!\n");				
						continue;
					}
					for (int i = 0; i < ret; i++) {
						count++;
						hup_depack(temp_buf[i], &app_ptr->hup_pack);
						printf("读到了data:%x\n",temp_buf[i]);
					}
				}
			}
			if (app_ptr->hup_pack.depack_status) {// 解析hup成功
				depack_success_handle(app_ptr, app_ptr->hup_pack.cmd_id, app_ptr->hup_pack.data_buf, app_ptr->hup_pack.length);
				count = 0;
			}
		}
	}
}

/******************************************************************************************
 * Function Name :  app_send_thread
 * Description   :  读send_fifo，将数据写入串口设备
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *app_send_thread(void *arg)
{
	int ret = 0;
	int count = 0;
	uint8_t temp_buf[255] = {0};
	st_app *app_ptr = (st_app *)arg;

	if (NULL == app_ptr) {
        dbg_perror("app_send_thread arg error.");
		exit(-1);
    }

	while (app_ptr->app_send_thd_flag) {
		memset(temp_buf, 0, sizeof(temp_buf));
		if (app_ptr->send_fifo.avail_len > 8 ) {
			/* 读FIFO */
			ret = fifo_read(&app_ptr->send_fifo, temp_buf, 2);
			if (-1 == ret) {
	            dbg_perror("fifo_read failed!\n");
	            continue;
	        }
			count = temp_buf[0];
			count = (count << 8) | temp_buf[1];
			memset(temp_buf, 0, sizeof(temp_buf));
			ret = fifo_read(&app_ptr->send_fifo, temp_buf, count);
			if (-1 == ret) {
	            dbg_perror("fifo_read failed!\n");
	            continue;
	        }
			/* 写入串口 */
			ret = uart_dev_write(app_ptr->uart_dev.uart_fd, temp_buf, ret);
			if (-1 == ret) {
				dbg_perror("uart_write failed!\n");
				continue;
			}
			dbg_printf(" uart_write %d .\n",ret); 
		}
	}
}

/******************************************************************************************
 * Function Name :  app_init
 * Description   :  初始化st_app结构体
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int app_init(st_app *app_ptr)
{
	int ret = 0;

	if (NULL == app_ptr) {
		dbg_perror("app_init arg error.");
		return -1;
	}

    app_ptr->app_recv_thd_flag   = false;
	app_ptr->app_handle_thd_flag = false;
	app_ptr->app_send_thd_flag   = false;
	memset(app_ptr->tid, 0, sizeof(app_ptr->tid));
	
	app_ptr->uart_dev.bps 		= UART_BPS;
	app_ptr->uart_dev.data_bit 	= UART_DATA_BIT;
	app_ptr->uart_dev.check_sum	= UART_CHECK_SUM;
	app_ptr->uart_dev.stop 		= UART_STOP;
	app_ptr->uart_dev.uart_fd 	= uart_dev_init(&app_ptr->uart_dev);
	if (-1 == app_ptr->uart_dev.uart_fd) {
		dbg_perror("uart_drv_init error. ");
		return -1;
	}

	ret = fifo_init(&app_ptr->recv_fifo, FIFO_SIZE);
	if (-1 == ret) {
		dbg_perror("recv_fifo init error.");
		return -1;
	}
	ret = fifo_init(&app_ptr->send_fifo, FIFO_SIZE);
	if (-1 == ret) {
		dbg_perror("send_fifo init error.");
		return -1;
	}
	
	return 0;
}

/******************************************************************************************
 * Function Name :  app_start
 * Description   :  启动app的线程的运行
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int app_start(st_app *app_ptr)
{
	int ret = 0;

	if (NULL == app_ptr) {
		dbg_perror("app_start arg error.");
		return -1;
	}

	app_ptr->app_recv_thd_flag   = true;
	app_ptr->app_handle_thd_flag = true;
	app_ptr->app_send_thd_flag   = true;
	ret = pthread_create(&app_ptr->tid[0], NULL, app_recv_thread, (void *)app_ptr);
	if (-1 == ret) {
		dbg_perror("recv_thread failed. \n");
		return -1;
	}
	ret = pthread_create(&app_ptr->tid[1], NULL, app_handle_thread, (void *)app_ptr);
	if (-1 == ret) {
		dbg_perror("handle_thread failed. \n");
		pthread_cancel(app_ptr->tid[0]);
		return -1;
	}
	ret = pthread_create(&app_ptr->tid[2], NULL, app_send_thread, (void *)app_ptr);
	if (-1 == ret) {
		dbg_perror("send_thread failed. \n");
		pthread_cancel(app_ptr->tid[0]);
		pthread_cancel(app_ptr->tid[1]);
		return -1;
	}

	return 0;
}

/******************************************************************************************
 * Function Name :  app_stop
 * Description   :  停止app的线程的运行并等待线程退出
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int app_stop(st_app *app_ptr)
{
	if (NULL == app_ptr) {
		dbg_perror("app_start arg error.");
		return -1;
	}
	
	app_ptr->app_recv_thd_flag   = false;
	app_ptr->app_handle_thd_flag = false;
	app_ptr->app_send_thd_flag   = false;

	pthread_join(app_ptr->tid[0], NULL);
	pthread_join(app_ptr->tid[1], NULL);
	pthread_join(app_ptr->tid[2], NULL);

	return 0;
}

/******************************************************************************************
 * Function Name :  app_deinit
 * Description   :  去初始化st_app结构体，释放fifo和串口资源
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int app_deinit(st_app *app_ptr)
{
	int ret = 0;

	if (NULL == app_ptr) {
		dbg_perror("app_start arg error.");
		return -1;
	}

	ret = fifo_deinit(&app_ptr->send_fifo);
	if (-1 == ret) {
		dbg_perror("send_fifo deinit error. ");
		return -1;
	}
	ret = fifo_deinit(&app_ptr->recv_fifo);
	if (-1 == ret) {
		dbg_perror("recv_fifo deinit error. ");
		return -1;
	}

	ret = uart_dev_deinit(&app_ptr->uart_dev);
	if (-1 == ret) {
		dbg_perror("uart_dev_deinit failed. ");
		return -1;
	}
	
	return 0;
}
