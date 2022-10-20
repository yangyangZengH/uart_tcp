#include "server_mod.h"

#define UART_BPS        115200
#define UART_DATA_BIT   8
#define UART_CHECK_SUM  'N'
#define UART_STOP       1


static int get_current_time(void)
{
    time_t times = time(NULL);
    return times;
}

/******************************************************************************************
 * Function Name :  server_sockfd_isread
 * Description   :  监听客户端是否可读
 * Parameters    :  readfd：文件描述符集合
 *                  fd：需要监听的文件描述符数组
 *                  num：需要监听的文件描述符数量
 *					sec：超时时间 单位 秒
 * 					usec：超时时间 单位 微妙
 * Returns       :  >0：表示被监视的文件描述符有变化。
 *					-1：表示select出错。
 *					 0：表示超时
*******************************************************************************************/
static int server_sockfd_isread(fd_set *readfd, int *fd, int num, int sec, int usec)
{
    int ret = 0;
    int count = 0;
    int fd_max = 0;
    struct timeval timeout;

    FD_ZERO(readfd);
    for (count = 0; count < num; count++) {
        FD_SET(fd[count], readfd);
        if (fd[count] > fd_max) {
           fd_max = fd[count];
        }
    }
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;
    ret = select(fd_max + 1, readfd, NULL, NULL, &timeout);

    return ret;
}

/******************************************************************************************
 * Function Name :  server_cmd_handle
 * Description   :  根据cmd命令类型处理相应命令
 * Parameters    :  sockfd：与该客户端通讯的套接字
 *                  server：st_tcp_server结构体指针
 *                  recv_buf：带hip协议的数据
 *                  len：数据长度
 * Returns       :  无
*******************************************************************************************/
static void server_cmd_handle(int sockfd, st_tcp_server *server, uint8_t *recv_buf, int len)
{
    uint8_t count = 0;
    st_node *pd = NULL;

    if (NULL == server || NULL == recv_buf) {
        dbg_perror("server_cmd_handle arg error.");
        return ;
    }

    for (count = 0; count < len; count++) {
        hip_depack(recv_buf[count], &server->hip);
    }

    pd = get_sockfd_node(server->l_mag_client_info, sockfd);
    switch (server->hip.commond) {
        case EM_HIP_USER_LOGIN:
            pd->data.recv_time = get_current_time();
            server_mod_send(server, sockfd, server->hip.commond, 0, NULL);
        case EM_HIP_KEEP_ALIVE:
            pd->data.recv_time = get_current_time();
            server_mod_send(server, sockfd, server->hip.commond, 0, NULL);
        break;
        case EM_HIP_UART_PASS_THROUGH:
            pd->data.recv_time = get_current_time();
            fifo_write(&server->send_fifo, recv_buf, len);
        break;
        default:
            dbg_perror("server_cmd_handle switch default !");
        break;
    }
}

/******************************************************************************************
 * Function Name :  server_bind_ip_port
 * Description   :  创建socket绑定监听
 * Parameters    :  server：st_tcp_server结构体指针
 * Returns       :  无
*******************************************************************************************/
static void server_bind_ip_port(st_tcp_server *server)
{
    int opt = 1;
    int sockfd = 0;
    struct sockaddr_in  server_addr;
	socklen_t addrlen = sizeof(server_addr);

    if (NULL == server) {
        dbg_perror("server_bind_ip_port arg error.");
        return ;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd){
       dbg_perror("server_bind_ip_port socket error.");
        return;
	}
    dbg_printf("server create socket success %d\n",sockfd);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;	
	server_addr.sin_port        = htons(server->server_info.server_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));
    if (bind(sockfd, (struct sockaddr *)&server_addr, addrlen) < 0) {
        dbg_perror("server_bind_ip_port bind error.");
        return ;
    }
    listen(sockfd, server->server_info.listen_num);
    server->server_info.sockfd = sockfd;
}

/******************************************************************************************
 * Function Name :  server_add_client_info
 * Description   :  将未连接过的客户端信息保存至链表，若已连接成功的客户端，将其连接断开
 * Parameters    :  server：st_tcp_server结构体指针
 *                  newfd：accept的返回值，与客户端通讯的套接字
 *                  ip_addr：客户端的ip地址
 * Returns       :  无
*******************************************************************************************/
static void server_add_client_info(st_tcp_server *server, int newfd, char *ip_addr)
{
    st_node *pn = NULL;

    if (NULL == server || NULL == ip_addr) {
        dbg_perror("server_add_client_info arg error.");
        return ;
    }
    
    // 遍历链表 该ip存在关掉旧的socket，存新的
    if (traversal_list(server->l_mag_client_info, ip_addr) == 0) {    // 存在         
        close(newfd);
        delete_node(server->l_mag_client_info, newfd);
    }

    pn = (st_node *)malloc(sizeof(*pn));
    if (NULL == pn) {
        dbg_perror("server_add_client_info malloc error.");
        return ;
    }
    pn->data.sockfd = newfd;
    pn->data.client_port = TCP_SERVER_PORT;       
    strncpy(pn->data.client_ip, ip_addr, strlen(ip_addr));
    pn->data.recv_time = get_current_time();       
    insert_node(server->l_mag_client_info, pn);
}

/******************************************************************************************
 * Function Name :  hup_pack_send_uart
 * Description   :  将数据hup封包发送至串口
 * Parameters    :  server：st_tcp_server结构体指针
 * Returns       :  无
*******************************************************************************************/
static void hup_pack_send_uart(st_tcp_server *server)
{
    int ret = 0;
    uint8_t temp_buf[255] = {0};

    if (NULL == server) {
        dbg_perror("hup_pack_send_uart arg error.");
        return;
    }

    ret = hup_pack(&server->hup, temp_buf);
	if (-1 == ret) {
		dbg_perror("hup_pack error!\n");
		return;
	}

    printf("uart_out:\n");
    for (int i = 0; i < server->hup.length + 6; i++) {
        printf("%x ",temp_buf[i]);
    }
    printf("\n");

    // /* 写入串口 */
    // ret = uart_dev_write(server->uart_dev.uart_fd, temp_buf, server->hup.length);
    // if (-1 == ret) {
    //     dbg_perror("uart_write error!\n");
    //     return;
    // }
    // dbg_printf(" uart_write %d .\n",ret); 
}

/******************************************************************************************
 * Function Name :  listen_server_connect
 * Description   :  监听客户端的连接申请
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *listen_server_connect(void *arg)
{
    fd_set readfd;
    int ret = 0;
    int newfd = 0;
    char ip_addr[32] = {0};
    struct sockaddr_in client_addr;
    int client_len = sizeof(client_addr);
    st_tcp_server *server = (st_tcp_server *)arg;

    if (NULL == arg) {
        dbg_perror("listen_server_connect arg error.");
        exit(-1);
    }

    while (server->server_listen_connect_thd_flag) {
        ret = server_sockfd_isread(&readfd, &server->server_info.sockfd, 1, 0, 500000);
        if (ret <= 0) {
            usleep(100000);
            continue;
        }

        newfd = accept(server->server_info.sockfd, (struct sockaddr *)&client_addr, &client_len);
        dbg_printf("accept success,fd = %d,the client's addr is %s port:%d\n",newfd,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_addr, sizeof(ip_addr));//网络字节序(二进制) 转 标准格式字符串(点分十进制) 
        server_add_client_info(server, newfd, ip_addr);
    }
}

static void recv_client_data(st_tcp_server *server, int connect_num, fd_set *readfd, int *fd)
{
    int ret = 0;
    int count = 0;
    int data_len = 0;
    st_node *pd = NULL;
    uint8_t recv_buf[TCP_FIFO_SIZE] = {0};

    if (NULL == server || NULL == fd) {
        dbg_perror("recv_client_data arg error.");
        return;
    }

    pd = server->l_mag_client_info->first;
    for (count = 0; count < connect_num; count++) {
        if (FD_ISSET(fd[count], readfd)) {
            memset(recv_buf, 0, sizeof(recv_buf));
            ret = recv(fd[count], recv_buf, HIP_HEAD_LENGTH, 0);
            if (ret < 0) {  
                dbg_perror("recv error.");
                close(fd[count]);
                delete_node(server->l_mag_client_info, fd[count]);
            } else if(ret == 0) {  
                dbg_perror("timeout.");
            } else {  
                dbg_printf("recv haed succeed.\n");  
                data_len = recv_buf[1];
                ret = recv(fd[count], recv_buf+HIP_HEAD_LENGTH, data_len, 0);
                if (ret < 0) {  
                    dbg_perror("recv error.");
                    close(fd[count]);
                    delete_node(server->l_mag_client_info, fd[count]);
                } else if(ret == 0) {  
                    dbg_perror("timeout.");
                } else {  
                    dbg_printf("recv body succeed.\n");                
                    pd->data.recv_time = get_current_time();
                    fifo_write(&server->recv_fifo, (uint8_t *)&fd[count], 1);
                    fifo_write(&server->recv_fifo, recv_buf, HIP_HEAD_LENGTH + data_len);  
                }              
            }
        }
        pd = pd->next;
    }
}

/******************************************************************************************
 * Function Name :  server_recv_thread
 * Description   :  接收客户端发送过来的数据，将数据转存到server->recv_fifo
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *server_recv_thread(void *arg)
{
    fd_set readfd;
    int ret = 0;
    int count = 0;
    st_node *pd = NULL;
    int connect_num = 0;
    int fd[LISTEN_NUM] = {0};
    
    st_tcp_server *server = (st_tcp_server *)arg;

    if (NULL == arg) {
        dbg_perror("server_recv_thread arg error.");
        exit(-1);
    }

    while(server->server_recv_thd_flag) {
        if (server->l_mag_client_info->first == NULL) {
            usleep(1000000);
            continue;
        }       
        connect_num = get_node_num(server->l_mag_client_info);
        pd = server->l_mag_client_info->first;
        for (count = 0; count < connect_num; count++) {
            fd[count] = pd->data.sockfd;
            pd = pd->next;
        }
        ret = server_sockfd_isread(&readfd, fd, connect_num, 0,500000);
        if (ret <= 0) {
            usleep(100000);
            continue;
        }    
        dbg_printf("server_sockfd_isread ok. \n");
        recv_client_data(server, connect_num, &readfd, fd);
    }
}

/******************************************************************************************
 * Function Name :  server_recv_handle_thread
 * Description   :  读取server->recv_fifo中的数据，处理cmd
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *server_recv_handle_thread(void *arg)
{
    int ret = 0; 
    int sockfd = 0;
    uint16_t data_len = 0;
    uint8_t recv_buf[255] = {0};
    uint8_t temp_buf[255] = {0};
    st_tcp_server *server = (st_tcp_server *)arg;

    if (NULL == arg) {
        dbg_perror("server_recv_handle_thread arg error.");
        exit(-1);
    }

    while(server->server_recv_handle_thd_flag) {
        if (server->recv_fifo.avail_len > 1 + HIP_HEAD_LENGTH) {
            fifo_read(&server->recv_fifo, (uint8_t *)&sockfd, 1);
            memset(recv_buf, 0, sizeof(recv_buf));
            ret = fifo_read(&server->recv_fifo, recv_buf, HIP_HEAD_LENGTH);
            if (-1 == ret) {
                dbg_perror("fifo_read error.");
                continue;
            }

            data_len = recv_buf[2] - HIP_HEAD_LENGTH;
            memset(temp_buf, 0, sizeof(temp_buf));
            ret = fifo_read(&server->recv_fifo, temp_buf, data_len);
            if (-1 == ret) {
                dbg_perror("fifo_read error.");
                continue;
            }
            memcpy(recv_buf + HIP_HEAD_LENGTH, temp_buf, data_len);
            server_cmd_handle(sockfd, server, recv_buf, HIP_HEAD_LENGTH + data_len);
        } 
    }
}

/******************************************************************************************
 * Function Name :  server_send_thread
 * Description   :  读取server->send_fifo的数据，按hup协议封包发往串口
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *server_send_thread(void *arg)
{
    int ret = 0; 
    uint8_t send_buf[255] = {0};
    uint8_t uart_data_length[2] = {0};
    st_tcp_server *server = (st_tcp_server *)arg;

    if (NULL == arg) {
        dbg_perror("server_send_thread arg error.");
        exit(-1);
    }

	while (server->server_send_thd_flag) {
		if (server->send_fifo.avail_len > HIP_HEAD_LENGTH) {
            memset(send_buf, 0, sizeof(send_buf));
			ret = fifo_read(&server->send_fifo, send_buf, HIP_HEAD_LENGTH);
            if (-1 == ret) {
                dbg_perror("fifo_read error.");
                continue;
            }
            server->hup.cmd_id = send_buf[1];
            memset(uart_data_length, 0, sizeof(uart_data_length));
            ret = fifo_read(&server->send_fifo, uart_data_length, 2);
            if (-1 == ret) {
                dbg_perror("fifo_read error.");
                continue;
            }   
            server->hup.length = (uart_data_length[0] << 8) | uart_data_length[1];
            memset(server->hup.data_buf, 0, sizeof(server->hup.data_buf));
            ret = fifo_read(&server->send_fifo, server->hup.data_buf, server->hup.length);
            if (-1 == ret) {
                dbg_perror("fifo_read error.");
                continue;
            }              
			hup_pack_send_uart(server);
		}
    }
}

/******************************************************************************************
 * Function Name :  server_keep_alive_thread
 * Description   :  心跳保持，管理服务器连接上的多个客户端
 * Parameters    :  arg：线程处理函数的参数
 * Returns       :  无
*******************************************************************************************/
void *server_keep_alive_thread(void *arg)
{
    int count = 0;
    int connect_num = 0;
    time_t cur_time;
    st_node *pt = NULL;
    st_tcp_server *server = (st_tcp_server *)arg;

    if (NULL == arg) {
        dbg_perror("server_keep_alive_thread arg error.");
        exit(-1);
    }
    
    while (server->server_keep_alive_thd_flag) {
        connect_num = get_node_num(server->l_mag_client_info);
        if (connect_num <= 0) {
            usleep(1000);
            continue;
        }
        pt = server->l_mag_client_info->first;
        for (count = 0; count < connect_num; count++) {
            cur_time = get_current_time();  
            if ((cur_time - pt->data.recv_time) > KEEP_ALIVE_TIMEOUT) {
                close(pt->data.sockfd);
                delete_node(server->l_mag_client_info, pt->data.sockfd);
                dbg_printf("sockfd:%d timeout\n",pt->data.sockfd);
            }
            pt = pt->next;     
        }
        usleep(500000);
    }
}

/******************************************************************************************
 * Function Name :  tcp_server_init
 * Description   :  初始化server结构体成员
 * Parameters    :  server：st_tcp_server结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_server_init(st_tcp_server *server)
{
    int ret = 0;

    if (NULL == server) {
        dbg_perror("tcp_server_init arg error.");
        return -1;
    }

    // server->uart_dev.bps 		= UART_BPS;
	// server->uart_dev.data_bit 	= UART_DATA_BIT;
	// server->uart_dev.check_sum	= UART_CHECK_SUM;
	// server->uart_dev.stop 		= UART_STOP;
	// server->uart_dev.uart_fd 	= uart_dev_init(&server->uart_dev);
    // if (-1 == server->uart_dev.uart_fd) {
	// 	dbg_perror("uart_drv_init error. ");
	// 	return -1;
	// }

    ret = fifo_init(&server->recv_fifo, TCP_FIFO_SIZE);
    if (-1 == ret) {
        dbg_perror("fifo_init error.");
        return -1;
    }
    ret = fifo_init(&server->send_fifo, TCP_FIFO_SIZE);
    if (-1 == ret) {
        dbg_perror("fifo_init error.");
        return -1;
    }
    
    server->server_recv_thd_flag           = false;
    server->server_send_thd_flag           = false;
    server->server_keep_alive_thd_flag     = false;
    server->server_recv_handle_thd_flag    = false;
    server->server_listen_connect_thd_flag = false;
    memset(server->tid, 0, sizeof(server->tid));

    server->l_mag_client_info       = create_linkedlist();
    server->server_info.sockfd      = 0;
    server->server_info.listen_num  = LISTEN_NUM;
    server->server_info.server_port = TCP_SERVER_PORT;
    server_bind_ip_port(server);

    return 0;
}

/******************************************************************************************
 * Function Name :  tcp_server_deinit
 * Description   :  去初始化server结构体成员
 * Parameters    :  server：st_tcp_server结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_server_deinit(st_tcp_server *server)
{
    int ret = 0;

    if (NULL == server) {
        dbg_perror("tcp_server_deinit arg error.");
        return -1;
    }
    
    destroy_linkedlist(server->l_mag_client_info);
    ret = fifo_deinit(&server->recv_fifo);
    if (-1 == ret) {
		dbg_perror("recv_fifo deinit error. ");
		return -1;
	}
    ret = fifo_deinit(&server->send_fifo);
    if (-1 == ret) {
		dbg_perror("send_fifo deinit error. ");
		return -1;
	}
    // ret = uart_dev_deinit(&server->uart_dev);
	// if (-1 == ret) {
	// 	dbg_perror("uart_dev_deinit error. ");
	// 	return -1;
	// }

    return 0;
}

/******************************************************************************************
 * Function Name :  tcp_server_start
 * Description   :  启动server的线程运行
 * Parameters    :  server：st_tcp_server结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_server_start(st_tcp_server *server)
{
    int ret = 0;

    if (NULL == server) {
        dbg_perror("tcp_server_start arg error.");
        return -1;
    }

    server->server_recv_thd_flag           = true;
    server->server_send_thd_flag           = true;
    server->server_keep_alive_thd_flag     = true;
    server->server_recv_handle_thd_flag    = true;
    server->server_listen_connect_thd_flag = true;

    ret = pthread_create(&server->tid[0], NULL, listen_server_connect, (void *)server);
    if (-1 == ret) {
        dbg_perror("pthread_create listen_server_connect error.");
        return -1;
    }
    ret = pthread_create(&server->tid[1], NULL, server_recv_thread, (void *)server);
    if (-1 == ret) {
        dbg_perror("pthread_create server_recv_thread error.");
        pthread_cancel(server->tid[0]);
        return -1;
    }
    ret = pthread_create(&server->tid[2], NULL, server_recv_handle_thread, (void *)server);
    if (-1 == ret) {
        dbg_perror("pthread_create server_recv_handle_thread error.");
        pthread_cancel(server->tid[0]);
        pthread_cancel(server->tid[1]);
        return -1;
    }
    ret = pthread_create(&server->tid[3], NULL, server_send_thread, (void *)server);
    if (-1 == ret) {
        dbg_perror("pthread_create server_send_thread error.");
        pthread_cancel(server->tid[0]);
        pthread_cancel(server->tid[1]);
        pthread_cancel(server->tid[2]);
        return -1;
    }
    ret = pthread_create(&server->tid[4], NULL, server_keep_alive_thread, (void *)server);
    if (-1 == ret) {
        dbg_perror("pthread_create server_keep_alive_thread error.");
        pthread_cancel(server->tid[0]);
        pthread_cancel(server->tid[1]);
        pthread_cancel(server->tid[2]);
        pthread_cancel(server->tid[3]);
        return -1;
    }
 
    return 0;
}

/******************************************************************************************
 * Function Name :  tcp_server_stop
 * Description   :  终止server的线程运行，等待线程退出
 * Parameters    :  server：st_tcp_server结构体指针
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int tcp_server_stop(st_tcp_server *server)
{
    if (NULL == server) {
        dbg_perror("tcp_server_stop arg error.");
        return -1;
    }

    server->server_recv_thd_flag           = false;
    server->server_send_thd_flag           = false;
    server->server_keep_alive_thd_flag     = false;
    server->server_recv_handle_thd_flag    = false;
    server->server_listen_connect_thd_flag = false;

    pthread_join(server->tid[0], NULL);
    pthread_join(server->tid[1], NULL);
    pthread_join(server->tid[2], NULL);
    pthread_join(server->tid[3], NULL);
    pthread_join(server->tid[4], NULL);

    return 0;
}

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
int server_mod_send(st_tcp_server *server, int sockfd, uint8_t cmd, int length, uint8_t *data)
{
    int ret = 0;
    uint8_t temp[255] = {0};

    ret = hip_pack_payload(&server->hip.hip_payload, cmd, length, data, 0, 0);
    if (-1 == ret) {
        dbg_perror("hip_pack_payload error.");
        return -1;
    }
    ret = hip_pack(cmd, &server->hip.hip_payload, temp);
    if (-1 == ret) {
        dbg_perror("hip_pack error.");
        return -1;
    }
    server->hip.length = temp[2];
    ret = send(sockfd, temp, server->hip.length, server->hip.length);
    if (ret < 0) {  
        dbg_perror("send error.");
        close(sockfd);
        delete_node(server->l_mag_client_info, sockfd);
    } else if(ret == 0) {  
        dbg_perror("timeout.");
    } else {  
        dbg_printf("send succeed.\n");    
        dbg_printf("server send succeed. sockfd:%d data_num:%d\n",sockfd, ret);            
    }         

    return 0;
}
