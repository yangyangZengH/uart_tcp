#include "app.h"
#include <sys/select.h> 


/******************************************************************************************
 * Function Name :  depack_success
 * Description   :  解包成功的后续操作，将需要应答PC的数据写入SEND_FIFO
 * Parameters    :  manage_thd：线程管理结构体指针
 * 					count：应答帧的长度
 * Returns       :  无
*******************************************************************************************/
static void depack_success(st_manage_thd *manage_thd, uint16_t count)
{
	int ret = 0;
	uint8_t temp_buf[255] = {0};
	uint8_t temp_len[2] = {0};

	if (NULL == manage_thd) 
        dbg_perror("depack_success failed!");

	printf("---------------------------------\n");
	printf("-----------cmd对应的操作---------\n");	
	printf("---------------------------------\n");

	memset(temp_buf, 0, sizeof(temp_buf));
	ret = hup_pack(&manage_thd->pack_msg, temp_buf);
	if (-1 == ret) {
		dbg_perror("hup_pack failed!\n");
		return;
	}
	dbg_printf("hup_pack succeed .\n");

	memset(temp_len, 0, sizeof(temp_len));
	temp_len[0] = count >> 8;
	temp_len[1] = count & 0xff;					
	ret = fifo_write(&manage_thd->send_fifo, temp_len, 2);
	if (-1 == ret) {
		dbg_perror("fifo_write length failed!\n");				
		return;
	}		

	ret = fifo_write(&manage_thd->send_fifo, temp_buf, count);	
	if (-1 == ret) {
		dbg_perror("fifo_write failed!\n");				
		return;
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

	ret = select(sizeof(read_fd) + 1, &read_fd, NULL, NULL, &timeout);
	return ret;
}

/******************************************************************************************
 * Function Name :  recv_thread
 * Description   :  A、读RECV_FIFO，逐个字节读取
 *					B、按照串口协议解析数据
 *					C、处理cmd id
 *					D、将需要应答PC的数据写入SEND_FIFO
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *recv_thread(void *arg)
{
	int ret = 0;
	int count = 0;
	uint8_t temp_buf[255] = {0};
	st_manage_thd *manage_thd = (st_manage_thd*)arg;

	if (NULL == manage_thd) {
        dbg_perror("app_start failed!");
    }

	while (manage_thd->worker_thd1_flag) {
		if (uart_dev_isread(manage_thd->uart_dev.uart_fd, 5, 0) > 0) {
			memset(temp_buf, 0, sizeof(temp_buf));
			ret = uart_dev_read(manage_thd->uart_dev.uart_fd, temp_buf, sizeof(temp_buf));
			if (-1 == ret) {
				dbg_perror("uart_dev_read failed!\n");
				continue;
			}
			/* 将从PC端读到的串口数据打印出来 */
			dbg_printf("uart_dev_read :\n"); 
			for (count = 0; count < ret; count++) {
				dbg_printf("%x ", temp_buf[count]);
			}
			dbg_printf("\n");	
 			/* 不解析，将读取到的数据直接weite()写进RECV_FIFO */
 			ret = fifo_write(&manage_thd->recv_fifo, temp_buf, count);
 			if (-1 == ret) {
				dbg_perror("fifo_write failed!\n");
				continue;
			}
			dbg_printf("write --%d-- FIFO .\n", ret);
 		}
	}
}

/******************************************************************************************
 * Function Name :  send_thread
 * Description   :  A、读RECV_FIFO，逐个字节读取
 *					B、按照串口协议解析数据
 *					C、处理cmd id
 *					D、将需要应答PC的数据写入SEND_FIFO
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *send_thread(void *arg)
{
	int ret = 0;
	int count = 0;
	uint8_t temp_buf[255] = {0};
	st_manage_thd *manage_thd = (st_manage_thd*)arg;

	if (NULL == manage_thd) {
        dbg_perror("app_start failed!");
    }

	while (manage_thd->worker_thd2_flag) {
		memset(temp_buf, 0, sizeof(temp_buf));
		if (manage_thd->send_fifo.avail_len > 8 ) {
			/* 读FIFO */
			ret = fifo_read(&manage_thd->send_fifo, temp_buf, 2);
			if (-1 == ret) {
	            dbg_perror("fifo_read failed!\n");
	            continue;
	        }
			count = temp_buf[0];
			count = (count << 8) | temp_buf[1];
			memset(temp_buf, 0, sizeof(temp_buf));
			ret = fifo_read(&manage_thd->send_fifo, temp_buf, count);
			if (-1 == ret) {
	            dbg_perror("fifo_read failed!\n");
	            continue;
	        }
			/* 写入串口 */
			ret = uart_dev_write(manage_thd->uart_dev.uart_fd, temp_buf, ret);
			if (-1 == ret) {
				dbg_perror("uart_write failed!\n");
				continue;
			}
			dbg_printf(" uart_write %d .\n",ret); 
		}
	}
}

/******************************************************************************************
 * Function Name :  handle_thread
 * Description   :  A、读RECV_FIFO，逐个字节读取
 *					B、按照串口协议解析数据
 *					C、处理cmd id
 *					D、将需要应答PC的数据写入SEND_FIFO
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *handle_thread(void *arg)
{
	int ret = 0;
	uint16_t count = 0;
	uint8_t temp_buf[255] = {0};
	uint8_t temp_len[2] = {0};
	uint8_t pack_data = 0;
	st_manage_thd *manage_thd = (st_manage_thd*)arg;

	if (NULL == manage_thd) {
        dbg_perror("app_start failed!");
    }

	while (manage_thd->handle_thd_flag) {
		if (manage_thd->recv_fifo.avail_len > 0) {
			memset(temp_buf, 0, sizeof(temp_buf));
			manage_thd->pack_msg.depack_status = false;			
			/* 读FIFO , 解析hup */
			if (!manage_thd->pack_msg.depack_status) {
				if (!manage_thd->pack_msg.length_status) {
					ret = fifo_read_byte(&manage_thd->recv_fifo, &pack_data);
					if (-1 == ret) {
						dbg_perror("fifo_read failed!\n");				
						continue;
					}
					count++;
					hup_depack(pack_data, &manage_thd->pack_msg);
				} else {
					ret = fifo_read(&manage_thd->recv_fifo, temp_buf, manage_thd->pack_msg.length);
					if (-1 == ret) {
						dbg_perror("fifo_read failed!\n");				
						continue;
					}
					for (int i = 0; i < ret; i++) {
						count++;
						hup_depack(temp_buf[i], &manage_thd->pack_msg);
						printf("读到了长度hup_depack:%d\n",temp_buf[i]);
					}
				}
			}
			if (manage_thd->pack_msg.depack_status) {
				depack_success(manage_thd, count);
				count = 0;
			}
		}
	}
}

/******************************************************************************************
 * Function Name :  app_init
 * Description   :  app模块初始化
 * Parameters    :  manage_thd：线程管理结构体指针
 * Returns       :  成功  0
 *					失败 -1
*******************************************************************************************/
int app_init(st_manage_thd *manage_thd)
{
	int ret = 0;

	if (NULL == manage_thd) {
        dbg_perror("app_start failed!");
        return -1;
    }

	ret = fifo_init(&manage_thd->recv_fifo, FIFO_SIZE);
	if (-1 == ret) {
		dbg_perror("fifo_init failed. ");
		return -1;
	}
	ret = fifo_init(&manage_thd->send_fifo, FIFO_SIZE);
	if (-1 == ret) {
		dbg_perror("fifo_init failed. ");
		return -1;
	}

	manage_thd->uart_dev.bps 		= UART_BPS;
	manage_thd->uart_dev.data_bit 	= UART_DATA_BIT;
	manage_thd->uart_dev.check_sum	= UART_CHECK_SUM;
	manage_thd->uart_dev.stop 		= UART_STOP;
	manage_thd->uart_dev.uart_fd 	= uart_dev_init(&manage_thd->uart_dev);
	if (-1 == manage_thd->uart_dev.uart_fd) {
		dbg_perror("uart_drv_init failed. ");
		return -1;
	}

	memset(manage_thd->worker_thd_tid, 0, sizeof(manage_thd->worker_thd_tid));

	manage_thd->worker_thd1_flag = false;
	manage_thd->worker_thd2_flag = false;
	manage_thd->handle_thd_flag  = false;

	return 0;
}

/******************************************************************************************
 * Function Name :  app_deinit
 * Description   :  app模块去初始化
 * Parameters    :  manage_thd：线程管理结构体指针
 * Returns       :  成功  0
 *					失败 -1
*******************************************************************************************/
int app_deinit(st_manage_thd *manage_thd)
{
	int ret = 0;

	if (NULL == manage_thd) {
        dbg_perror("app_deinit failed!");
        return -1;
    }

	ret = uart_dev_deinit(&manage_thd->uart_dev);
	if (-1 == ret) {
		dbg_perror("app_deinit failed. ");
		return -1;
	}

	ret = fifo_deinit(&manage_thd->send_fifo);
	if (-1 == ret) {
		dbg_perror("app_deinit failed. ");
		return -1;
	}

	ret = fifo_deinit(&manage_thd->recv_fifo);
	if (-1 == ret) {
		dbg_perror("app_deinit failed. ");
		return -1;
	}

	return 0;
}

/******************************************************************************************
 * Function Name :  app_start
 * Description   :  启动app模块，拉起3个工作线程
 * Parameters    :  manage_thd：线程管理结构体指针
 * Returns       :  成功  0
 *					失败 -1
*******************************************************************************************/
int app_start(st_manage_thd *manage_thd)
{
	int ret = 0;

	if (NULL == manage_thd) {
        dbg_perror("app_start failed!");
        return -1;
    }

	manage_thd->worker_thd1_flag = true;
	manage_thd->worker_thd2_flag = true;
	manage_thd->handle_thd_flag  = true;

	ret = pthread_create(&manage_thd->worker_thd_tid[0], NULL, recv_thread, (void *)manage_thd);
	if (-1 == ret) {
		dbg_perror("pthread1_create failed. \n");
		return -1;
	}
	ret = pthread_create(&manage_thd->worker_thd_tid[1], NULL, handle_thread, (void *)manage_thd);
	if (-1 == ret) {
		dbg_perror("pthread2_create failed. \n");
		pthread_cancel(manage_thd->worker_thd_tid[0]);
		return -1;
	}
	ret = pthread_create(&manage_thd->worker_thd_tid[2], NULL, send_thread, (void *)manage_thd);
	if (-1 == ret) {
		dbg_perror("pthread2_create failed. \n");
		pthread_cancel(manage_thd->worker_thd_tid[0]);
		pthread_cancel(manage_thd->worker_thd_tid[1]);
		return -1;
	}

	return 0;
}

/******************************************************************************************
 * Function Name :  app_stop
 * Description   :  停止app模块，终止线程运行
 * Parameters    :  manage_thd：线程管理结构体指针
 * Returns       :  成功  0
 *					失败 -1
*******************************************************************************************/
int app_stop(st_manage_thd *manage_thd)
{
	if (NULL == manage_thd) {
        dbg_perror("app_stop failed!");
        return -1;
    }

	manage_thd->worker_thd1_flag = false;
	manage_thd->worker_thd2_flag = false;
	manage_thd->handle_thd_flag  = false;

	pthread_join(manage_thd->worker_thd_tid[0], NULL);
	pthread_join(manage_thd->worker_thd_tid[1], NULL);
	pthread_join(manage_thd->worker_thd_tid[2], NULL);

	return 0;
}