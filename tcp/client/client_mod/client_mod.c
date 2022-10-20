#include "client_mod.h"





static int get_current_time(void)
{
    time_t times = time(NULL);
    return times;
}

/******************************************************************************************
 * Function Name :  client_sockfd_isread
 * Description   :  监听服务端是否可读
 * Parameters    :  fd：需要监听的文件描述符
 * 					sec：超时时间 单位 秒
 * 					usec：超时时间 单位 微妙
 * Returns       :  >0：表示被监视的文件描述符有变化。
 *					-1：表示select出错。
 *					 0：表示超时
*******************************************************************************************/
static int client_sockfd_isread(int fd, int sec, int usec)
{
    int ret = 0;
    struct timeval timeout;

    fd_set read_fd;
    FD_ZERO(&read_fd);
    FD_SET(fd, &read_fd);
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;
    ret = select(fd + 1, &read_fd, NULL, NULL, &timeout);

    return ret;
}

/******************************************************************************************
 * Function Name :  client_cmd_handle
 * Description   :  根据cmd命令类型处理相应命令
 * Parameters    :  client：st_tcp_client结构体指针
 *                  recv_buf：带hip协议的数据
 *                  len：数据长度
 * Returns       :  无
*******************************************************************************************/
static void client_cmd_handle(st_tcp_client *client, uint8_t *recv_buf, int len)
{
    uint8_t count = 0;

    if (NULL == client || NULL == recv_buf) {
        dbg_perror("client_cmd_handle arg error.");
        return ;
    }
    
    for (count = 0; count < len; count++) {
        hip_depack(recv_buf[count], &client->hip);
    }
    switch (client->hip.commond) {
        case EM_HIP_USER_LOGIN:
            if (client->hip.hip_payload.login_state == 0) {
                client->client_status.connect_state = true;
                client->client_status.recv_time = get_current_time();
            }
        break;
        case EM_HIP_KEEP_ALIVE:
            client->client_status.recv_frame++;
            client->client_status.recv_time = get_current_time();
        break;
        default:
            dbg_perror("client_cmd_handle switch default !");
        break;
    }
}

/******************************************************************************************
 * Function Name :  client_close_connect
 * Description   :  客户端关闭与服务端的连接
 * Parameters    :  client：st_tcp_client结构体指针
 * Returns       :  无
*******************************************************************************************/
static void client_close_connect(st_tcp_client *client)
{
    if (NULL == client) {
        dbg_perror("client_close_connect arg error.");
        return ;
    }
    
    if (client->client_info.sockfd > 0) {
        close(client->client_info.sockfd);
        client->client_info.sockfd = 0;
    }
    client->client_status.connect_state = false;
}

/******************************************************************************************
 * Function Name :  client_connect_server
 * Description   :  与服务端建立连接，更新客户端的心跳时间
 * Parameters    :  client：st_tcp_client结构体指针
 * Returns       :  无
*******************************************************************************************/
static void client_connect_server(st_tcp_client *client)
{
    int ret = 0;
    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(server_addr);

    if (NULL == client) {
        dbg_perror("client_reconnection arg error.");
        return ;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(client->client_info.server_port);
    server_addr.sin_addr.s_addr = inet_addr(client->client_info.server_ip);

    client->client_info.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == client->client_info.sockfd) {
        dbg_perror("client_connect_server socket error.");
        return ;
    }
    dbg_printf("create socket success %d\n",client->client_info.sockfd);

    ret = connect(client->client_info.sockfd, (struct sockaddr *)&server_addr, addrlen);
    if (-1 == ret) {
        perror("client_connect_server connect error.");
        close(client->client_info.sockfd);
        return ;
    }
    dbg_printf("connect success\n");

    //client->client_status.connect_state = true;
    client->client_status.send_time = get_current_time();
    //client->client_status.recv_time = get_current_time();
}

/******************************************************************************************
 * Function Name :  client_send_thread
 * Description   :  读取client->send_fifo的数据，发送给服务端
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *client_send_thread(void *arg)
{
    int ret = 0;
    int send_data = 0;
    int data_len = 0;
    uint8_t send_buf[255] = {0};
    uint8_t temp_buf[255] = {0};
    st_tcp_client *client = (st_tcp_client *)arg;

    if (NULL == arg) {
        dbg_perror("client_send_thread arg error.");
        exit(-1);
    }

    while(client->client_send_thd_flag) {
        if (client->send_fifo.avail_len > HIP_HEAD_LENGTH) {
            memset(send_buf, 0, sizeof(send_buf));
            ret = fifo_read(&client->send_fifo, send_buf, HIP_HEAD_LENGTH);
            if (-1 == ret) {
                dbg_perror("fifo_read error.");
                continue;
            }
            send_data += ret;

            data_len = send_buf[2] - HIP_HEAD_LENGTH;
            memset(temp_buf, 0, sizeof(temp_buf));
            ret = fifo_read(&client->send_fifo, temp_buf, data_len);
            if (-1 == ret) {
                dbg_perror("fifo_read error.");
                continue;
            }
            send_data += ret;
            
            memcpy(send_buf + HIP_HEAD_LENGTH, temp_buf, data_len);
            ret = send(client->client_info.sockfd, send_buf, send_data, 0);
            client->client_status.send_time = get_current_time();
            dbg_printf("tcp client send data:%d\n", ret);
            send_data = 0;
        } else {
            usleep(10000);
        }
    }
}
    
/******************************************************************************************
 * Function Name :  client_recv_thread
 * Description   :  接收服务端发送来的数据，转存到client->recv_fifo
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *client_recv_thread(void *arg)
{
    int ret = 0;
    uint8_t recv_buf[1024] = {0};
    st_tcp_client *client = (st_tcp_client *)arg;

    if (NULL == arg) {
        dbg_perror("client_recv_thread arg error.");
        exit(-1);
    }

    while(client->client_recv_thd_flag) {
        if (client->client_info.sockfd <= 0) 
            continue;
        ret = client_sockfd_isread(client->client_info.sockfd, 3, 0);
        if (ret < 0) {
            dbg_perror("client_sockfd_isread error.");
            continue;
        } else if (ret == 0) {
            dbg_printf("timeout.\n");
            usleep(1000);
            continue;
        }
        memset(recv_buf, 0, sizeof(recv_buf));
        ret = recv(client->client_info.sockfd, recv_buf, sizeof(recv_buf), 0);
        if (ret < 0) {
            dbg_perror("client recv error.");
            continue;
        } else if (ret == 0) {// 另一端关闭了此连接
            dbg_printf("The connection has been disconnected.\n");
            client_connect_server(client);
            continue;
        }
        client->client_status.recv_time = get_current_time();
        fifo_write(&client->recv_fifo, recv_buf, ret);
    }
}

/******************************************************************************************
 * Function Name :  client_recv_handle_thread
 * Description   :  读取client->recv_fifo，解析hip并处理cmd命令
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *client_recv_handle_thread(void *arg)
{
    int ret = 0; 
    int data_len = 0;
    uint8_t recv_buf[255] = {0};
    uint8_t temp_buf[255] = {0};
    st_tcp_client *client = (st_tcp_client *)arg;

    if (NULL == arg) {
        dbg_perror("client_recv_handle_thread arg error.");
        exit(-1);
    }

    while(client->client_recv_handle_thd_flag) {
        if (client->recv_fifo.avail_len > HIP_HEAD_LENGTH) {
            memset(recv_buf, 0, sizeof(recv_buf));
            ret = fifo_read(&client->recv_fifo, recv_buf, HIP_HEAD_LENGTH);
            if (-1 == ret) {
                dbg_perror("fifo_read error.");
                continue;
            }
            data_len = recv_buf[2] - HIP_HEAD_LENGTH; 
            memset(temp_buf, 0, sizeof(temp_buf));
            ret = fifo_read(&client->recv_fifo, temp_buf, data_len);
            if (-1 == ret) {
                dbg_perror("fifo_read error.");
                continue;
            }
            memcpy(recv_buf + HIP_HEAD_LENGTH, temp_buf, data_len);
            client_cmd_handle(client, recv_buf, HIP_HEAD_LENGTH + data_len);
        } 
    }
}

/******************************************************************************************
 * Function Name :  client_keep_alive_thread
 * Description   :  心跳保持，定时向服务端发送心跳包，超时重连
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *client_keep_alive_thread(void *arg)
{
    time_t cur_time = 0;
    st_tcp_client *client = (st_tcp_client *)arg;

    if (NULL == arg) {
        dbg_perror("client_keep_alive_thread arg error.");
        exit(-1);
    }

    while(client->client_keep_alive_thd_flag) {
        if (client->client_status.connect_state) {
            cur_time = get_current_time();
            if (abs(cur_time - client->client_status.send_time >= SENG_KEEP_ALIVE_TIME)) {
                // 发送心跳包
                client->client_status.send_frame++;
                client->client_status.send_time = get_current_time();
                client_mod_send(client, EM_HIP_KEEP_ALIVE, 0, NULL);  
            }
            if (abs(cur_time - client->client_status.recv_time >= KEEP_ALIVE_TIMEOUT)) {
                // 没有按时收到心跳包
                client_close_connect(client);
                client_connect_server(client);
            }
        } else {
            usleep(500000);
        }
    }
}

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
int client_mod_send(st_tcp_client *client, uint8_t cmd, uint16_t length, uint8_t *data)
{
    int ret = 0;
    uint8_t temp[255] = {0};

    if (NULL == client) {
        dbg_perror("client_mod_send arg error.");
        return -1;
    }

    ret = hip_pack_payload(&client->hip.hip_payload, cmd, length, data, \
            client->client_status.send_frame, client->client_status.recv_frame);
    if (-1 == ret) {
        dbg_perror("hip_pack_payload error.");
        return -1;
    }

    memset(temp, 0, sizeof(temp));
    ret = hip_pack(cmd, &client->hip.hip_payload, temp);
    if (-1 == ret) {
        dbg_perror("hip_pack error.");
        return -1;
    }

    client->hip.length = temp[2];
    ret = fifo_write(&client->send_fifo, temp, client->hip.length);
    if (-1 == ret) {
        dbg_perror("fifo_write error.");
        return -1;
    }
}

/******************************************************************************************
 * Function Name :  tcp_client_init
 * Description   :  初始化client结构体成员，与服务端建立连接
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_client_init(st_tcp_client *client)
{
    int ret = 0;

    if (NULL == client) {
        dbg_perror("tcp_client_init arg error.");
        return -1;
    }

    ret = fifo_init(&client->recv_fifo, TCP_FIFO_SIZE);
    if (-1 == ret) {
        dbg_perror("fifo_init error.");
        return -1;
    }
    ret = fifo_init(&client->send_fifo, TCP_FIFO_SIZE);
    if (-1 == ret) {
        dbg_perror("fifo_init error.");
        return -1;
    }
    client->client_send_thd_flag        = false;
    client->client_recv_thd_flag        = false;
    client->client_recv_handle_thd_flag = false;
    client->client_keep_alive_thd_flag  = false;  

    memset(client->tid, 0, sizeof(client->tid));
    memcpy(client->client_info.server_ip, TCP_SERVER_IP, TCP_IP_LENGTH);
    client->client_info.server_port     = TCP_SERVER_PORT;
    client->client_info.sockfd          = 0;
    client->client_status.recv_frame    = 0;
    client->client_status.send_frame    = 0;
    client->client_status.connect_state = false;
    // 连接
    client_connect_server(client);

    return 0;
}

/******************************************************************************************
 * Function Name :  tcp_client_deinit
 * Description   :  去初始化client，释放资源，关闭客户端连接
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_client_deinit(st_tcp_client *client)
{
    if (NULL == client) {
        dbg_perror("tcp_client_deinit arg error.");
        return -1;
    }
    
    fifo_deinit(&client->recv_fifo);
    fifo_deinit(&client->send_fifo);
    client_close_connect(client);

    return 0;
}

/******************************************************************************************
 * Function Name :  tcp_client_start
 * Description   :  启动client的线程运行
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_client_start(st_tcp_client *client)
{
    int ret = 0;

    if (NULL == client) {
        dbg_perror("tcp_client_start arg error.");
        return -1;
    }

    client->client_send_thd_flag           = true;
    client->client_recv_thd_flag           = true;
    client->client_recv_handle_thd_flag    = true;
    client->client_keep_alive_thd_flag     = true;
    ret = pthread_create(&client->tid[0], NULL, client_send_thread, (void *)client);
    if (-1 == ret) {
        dbg_perror("pthread_create client_send_thread error.");
        return -1;
    }
    ret = pthread_create(&client->tid[1], NULL, client_recv_thread, (void *)client);
    if (-1 == ret) {
        dbg_perror("pthread_create client_recv_thread error.");
        pthread_cancel(client->tid[0]);
        return -1;
    }
    ret = pthread_create(&client->tid[2], NULL, client_recv_handle_thread, (void *)client);
    if (-1 == ret) {
        dbg_perror("pthread_create client_recv_handle_thread error.");
        pthread_cancel(client->tid[0]);
        pthread_cancel(client->tid[1]);
        return -1;
    }
    ret = pthread_create(&client->tid[3], NULL, client_keep_alive_thread, (void *)client);
    if (-1 == ret) {
        dbg_perror("pthread_create client_keep_alive_thread error.");
        pthread_cancel(client->tid[0]);
        pthread_cancel(client->tid[1]);
        pthread_cancel(client->tid[2]);
        return -1;
    }

    return 0;
}

/******************************************************************************************
 * Function Name :  tcp_client_stop
 * Description   :  终止client的线程运行，等待线程退出
 * Parameters    :  app_ptr：st_app结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_client_stop(st_tcp_client *client)
{
    if (NULL == client) {
        dbg_perror("tcp_client_stop arg error.");
        return -1;
    }

    client->client_send_thd_flag        = false;
    client->client_recv_thd_flag        = false;
    client->client_recv_handle_thd_flag = false;
    client->client_keep_alive_thd_flag  = false;

    pthread_join(client->tid[0], NULL);
    pthread_join(client->tid[1], NULL);
    pthread_join(client->tid[2], NULL);
    pthread_join(client->tid[3], NULL);

    return 0;
}
