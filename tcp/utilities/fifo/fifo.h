#ifndef __FIFO_H__
#define __FIFO_H__


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

typedef struct st_fifo_buf 
{            
    int avail_len;              //当前fifo中有多少个数据
    int buffer_size;            //缓存总空间大小
    int wr_place;               //当前写入字符位置的下一个位置
    int rd_place;               //当前读取字符位置的下一个位置
    uint8_t *buffer_addr;       //缓存空间地址

    pthread_mutex_t mtx;
    pthread_cond_t  cv;
}st_fifo_buf;




/******************************************************************************************
 * Function Name :  fifo_init
 * Description   :  fifo结构体初始化
 * Parameters    :  fifo_ptr：fifo结构体指针
                    fifo_size: fifo结构体大小
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int fifo_init(st_fifo_buf *fifo_ptr, int fifo_size);

/******************************************************************************************
 * Function Name :  fifo_deinit
 * Description   :  释放fifo中的堆空间
 * Parameters    :  fifo_ptr：fifo结构体指针
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int fifo_deinit(st_fifo_buf *fifo_ptr);

/******************************************************************************************
 * Function Name :  fifo_read
 * Description   :  读取fifo中的内容，并返回读取到的字符数
 * Parameters    :  fifo_ptr：fifo结构体指针
                    read_save_buf: 存放读取到的内容的空间
                    read_size：要读取字符的大小
 * Returns       :  成功返回 读取到的字符数
                    失败返回 -1
*******************************************************************************************/
int fifo_read(st_fifo_buf *fifo_ptr, uint8_t *read_save_buf, int read_size);

/******************************************************************************************
 * Function Name :  fifo_write
 * Description   :  往fifo中写入字符内容，并返回写入的字符数
 * Parameters    :  fifo_ptr：fifo结构体指针
                    write_buf: 要写入的字符
                    write_size：要写入字符的大小
 * Returns       :  成功返回 写入成功的字符数
                    失败返回 -1
*******************************************************************************************/
int fifo_write(st_fifo_buf *fifo_ptr, uint8_t *write_buf, int write_size);

/******************************************************************************************
 * Function Name :  fifo_read_byte
 * Description   :  从fifo中读取一个字节
 * Parameters    :  fifo_ptr：fifo结构体指针
 *                  read_save_buf：存放读取到的数据
 * Returns       :  成功返回 读取到的字符数
                    失败返回 -1
*******************************************************************************************/
int fifo_read_byte(st_fifo_buf *fifo_ptr, uint8_t *read_save_buf);

/******************************************************************************************
 * Function Name :  fifo_write_byte
 * Description   :  往fifo中写入一个字节
 * Parameters    :  fifo_ptr：fifo结构体指针
 *                  read_save_buf：待写入的数据
 * Returns       :  成功返回 写入成功的字符数
                    失败返回 -1
*******************************************************************************************/
int fifo_write_byte(st_fifo_buf *fifo_ptr, uint8_t write_buf);

#endif

