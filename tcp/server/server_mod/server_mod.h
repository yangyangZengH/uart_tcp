#ifndef __SERVER_MOD_H__
#define __SERVER_MOD_H__


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "fifo.h"
#include "hip.h"
#include "hup.h"
#include "common.h"
#include "linkedlist.h"
#include "uart_dev.h"


#define     TCP_FIFO_SIZE           1024
#define     TCP_SERVER_PORT         8787
#define     LISTEN_NUM              20
#define     KEEP_ALIVE_TIMEOUT      40



typedef struct st_tcp_server_info {
    int listen_num;                         // 监听数量
	int sockfd;                             // 套接字
	int server_port;                        // 端口
	char server_ip[32];	                    // ip地址
}st_tcp_server_info;

typedef struct st_tcp_server {
    pthread_t tid[5];                       // 线程ID数组
    bool server_listen_connect_thd_flag;    // 线程循环标志位
    bool server_recv_thd_flag;
    bool server_recv_handle_thd_flag;
    bool server_keep_alive_thd_flag;
    bool server_send_thd_flag;

    st_fifo_buf recv_fifo;                  // 接收fifo
    st_fifo_buf send_fifo;                  // 发送fifo
    st_hip_pack hip;                        // hip协议结构体
    st_hup_pack hup;                        // hup协议结构体
    st_uart_dev uart_dev;                   // 串口设备结构体

    st_linkedlist *l_mag_client_info;       // 管理客户端信息的链表
    st_tcp_server_info server_info;         // 服务端信息结构体
}st_tcp_server;



/******************************************************************************************
 * Function Name :  tcp_server_init
 * Description   :  初始化server结构体成员
 * Parameters    :  server：st_tcp_server结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_server_init(st_tcp_server *server);

/******************************************************************************************
 * Function Name :  tcp_server_deinit
 * Description   :  去初始化server结构体成员
 * Parameters    :  server：st_tcp_server结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_server_deinit(st_tcp_server *server);

/******************************************************************************************
 * Function Name :  tcp_server_start
 * Description   :  启动server的线程运行
 * Parameters    :  server：st_tcp_server结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_server_start(st_tcp_server *server);

/******************************************************************************************
 * Function Name :  tcp_server_stop
 * Description   :  终止server的线程运行，等待线程退出
 * Parameters    :  server：st_tcp_server结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_server_stop(st_tcp_server *server);

/******************************************************************************************
 * Function Name :  server_mod_send
 * Description   :  服务端发送，将数据进行hip封包，发送给指定sockfd
 * Parameters    :  server：st_tcp_server结构体指针
 *                  sockfd：用于tcp通讯的套接字句柄
 *                  cmd：待发送数据的命令类型
 *                  length: 待发送数据的长度
 *                  data：待发送的数据
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int server_mod_send(st_tcp_server *server, int sockfd, uint8_t cmd, int length, uint8_t *data);






#endif