#ifndef __APP_H__
#define __APP_H__

#include "fifo.h"
#include "uart_dev.h"
#include "client_mod.h"
#include "hup.h"
#include "common.h"


#define UART_BPS        115200
#define UART_DATA_BIT   8
#define UART_CHECK_SUM  'N'
#define UART_STOP       1
#define FIFO_SIZE       255




typedef struct st_app
{
    pthread_t tid[3];           // 线程ID数组
    bool app_recv_thd_flag;     // 线程循环标志位
	bool app_handle_thd_flag;
	bool app_send_thd_flag;

    st_fifo_buf recv_fifo;      // 接收fifo
    st_fifo_buf send_fifo;      // 发送fifo
    st_uart_dev uart_dev;       // 串口设备结构体
    st_hup_pack hup_pack;       // hup协议结构体
	st_tcp_client client;       // tcp_client结构体
}st_app;



/******************************************************************************************
 * Function Name :  app_init
 * Description   :  初始化st_app结构体
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int app_init(st_app *app);

/******************************************************************************************
 * Function Name :  app_start
 * Description   :  启动app的线程的运行
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int app_start(st_app *app);

/******************************************************************************************
 * Function Name :  app_stop
 * Description   :  停止app的线程的运行并等待线程退出
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int app_stop(st_app *app);

/******************************************************************************************
 * Function Name :  app_deinit
 * Description   :  去初始化st_app结构体，释放fifo和串口资源
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int app_deinit(st_app *app);

#endif //__APP_H__
