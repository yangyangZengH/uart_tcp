#ifndef __UART_DEV_H__
#define __UART_DEV_H__

#include <termios.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "common.h"

#define UART_DEV    "/dev/ttyUSB0"

typedef struct st_uart_dev{
    int uart_fd;
    int bps;
    int data_bit;
    char check_sum;
    int stop;
}st_uart_dev;

/******************************************************************************************
 * Function Name :  uart_dev_init
 * Description   :  打开串口对应的设备节点，串口初始化，配置波特率，数据位，奇偶校验位和停止位
 * Parameters    :  
 * Returns       :  成功返回 fd
					失败返回 -1
*******************************************************************************************/
int uart_dev_init(st_uart_dev *uart_dev);

/******************************************************************************************
 * Function Name :  uart_dev_read
 * Description   :  读取串口中的内容，并返回读取到的字符数
 * Parameters    :  fd：串口设备的文件描述符
 					data_buf: 存放读取到的内容的空间
 					len: 要读取的字符长度
 * Returns       :  成功 返回读取的字符数
 					失败 -1
 *******************************************************************************************/
int uart_dev_read(int fd, uint8_t *data_buf, int len);

/******************************************************************************************
 * Function Name :  uart_dev_write
 * Description   :  往串口中写入字符内容，并返回写入的字符数
 * Parameters    :  fd：串口设备的文件描述符
 					data_buf: 要写入的字符
 					len: 要写入的字符长度
 * Returns       :  成功 返回写入的字符数
 					失败 -1
 *******************************************************************************************/
int uart_dev_write(int fd, uint8_t *data_buf, int len);

/******************************************************************************************
 * Function Name :  uart_dev_deinit
 * Description   :  串口去初始化，关闭串口对应的设备节点
 * Parameters    :  fd：串口设备的文件描述符
 * Returns       :  成功 0
 					失败 -1
 *******************************************************************************************/
int uart_dev_deinit(st_uart_dev *uart_dev);


#endif
