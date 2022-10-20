#ifndef __APP_H__
#define __APP_H__

#include "fifo.h"
#include "hup.h"
#include "uart_dev.h"
#include "common.h"

typedef struct st_manage_thd
{
    pthread_t worker_thd_tid[3];

    bool worker_thd1_flag;
    bool worker_thd2_flag;
    bool handle_thd_flag;

    st_uart_dev uart_dev;
    st_pack_msg pack_msg;
    st_fifo_buf recv_fifo;
    st_fifo_buf send_fifo;

}st_manage_thd;



/******************************************************************************************
 * Function Name :  app_init
 * Description   :  app模块初始化
 * Parameters    :  manage_thd：线程管理结构体指针
 * Returns       :  成功  0
 *					失败 -1
*******************************************************************************************/
int app_init(st_manage_thd *manage_thd);

/******************************************************************************************
 * Function Name :  app_deinit
 * Description   :  app模块去初始化
 * Parameters    :  manage_thd：线程管理结构体指针
 * Returns       :  成功  0
 *					失败 -1
*******************************************************************************************/
int app_deinit(st_manage_thd *manage_thd);

/******************************************************************************************
 * Function Name :  app_start
 * Description   :  启动app模块，拉起3个工作线程
 * Parameters    :  manage_thd：线程管理结构体指针
 * Returns       :  成功  0
 *					失败 -1
*******************************************************************************************/
int app_start(st_manage_thd *manage_thd);

/******************************************************************************************
 * Function Name :  app_stop
 * Description   :  停止app模块，终止线程运行
 * Parameters    :  manage_thd：线程管理结构体指针
 * Returns       :  成功  0
 *					失败 -1
*******************************************************************************************/
int app_stop(st_manage_thd *manage_thd);

#endif //__APP_H__
