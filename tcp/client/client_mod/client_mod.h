#ifndef __CLIENT_MOD_H__
#define __CLIENT_MOD_H__


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "fifo.h"
#include "hip.h"
#include "common.h"



#define     TCP_SERVER_PORT         8787
#define     TCP_SERVER_IP           "127.0.0.1"
#define     TCP_FIFO_SIZE           255
#define     SENG_KEEP_ALIVE_TIME    15
#define     KEEP_ALIVE_TIMEOUT      35
#define     TCP_IP_LENGTH           32



typedef struct st_tcp_client_info {
	int sockfd;                         // 套接字
	int server_port;                    // 端口
	char server_ip[TCP_IP_LENGTH];	    // ip地址
}st_tcp_client_info;


typedef struct st_client_status {
	unsigned int send_time;             // 客户端发送时间
	unsigned int recv_time;             // 客户端接收时间
	bool connect_state;                 // 客户端连接状态
    float recv_frame;                   // 心跳接收次数
    float send_frame;                   // 心跳发送次数
}st_client_status;

typedef struct st_tcp_client {
    pthread_t tid[4];                   // 线程ID数组
    bool client_send_thd_flag;          // 线程循环标志位
    bool client_recv_thd_flag;
    bool client_recv_handle_thd_flag;
    bool client_keep_alive_thd_flag;

    st_fifo_buf recv_fifo;              // 接收fifo
    st_fifo_buf send_fifo;              // 发送fifo
    st_hip_pack hip;                    // hip协议结构体
    st_tcp_client_info client_info;     // 客户端信息结构体
    st_client_status client_status;     // 客户端状态结构体
}st_tcp_client;



/******************************************************************************************
 * Function Name :  tcp_client_init
 * Description   :  初始化client结构体成员，与服务端建立连接
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_client_init(st_tcp_client *client);

/******************************************************************************************
 * Function Name :  tcp_client_deinit
 * Description   :  去初始化client，释放资源，关闭客户端连接
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_client_deinit(st_tcp_client *client);

/******************************************************************************************
 * Function Name :  tcp_client_start
 * Description   :  启动client的线程运行
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_client_start(st_tcp_client *client);

/******************************************************************************************
 * Function Name :  tcp_client_stop
 * Description   :  终止client的线程运行，等待线程退出
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_client_stop(st_tcp_client *client);

/******************************************************************************************
 * Function Name :  client_mod_send
 * Description   :  客户端发送，将数据hip封包存client->send_fifo
 * Parameters    :  app_ptr：st_app结构体指针
 *                  cmd：待发送数据的命令类型
 *                  length：待发送数据的长度
 *                  data：待发送的数据
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int client_mod_send(st_tcp_client *client, uint8_t cmd, uint16_t length, uint8_t *data);


#endif