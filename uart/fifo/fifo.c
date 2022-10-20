#include "fifo.h"



/******************************************************************************************
 * Function Name :  fifo_read
 * Description   :  读取fifo中的内容，并返回读取到的字符数
 * Parameters    :  fifo_ptr：fifo结构体指针
                    read_save_buf: 存放读取到的内容的空间
                    read_size：要读取字符的大小
 * Returns       :  成功返回 读取到的字符数
                    失败返回 -1
*******************************************************************************************/
int fifo_read(st_fifo_buf *fifo_ptr, uint8_t *read_save_buf, int read_size)
{
    int count = 0;

    if (NULL == fifo_ptr || NULL == read_save_buf || read_size <= 0) {
        dbg_perror("fifo_read failed!\n");
        return -1;
    }

    pthread_mutex_lock(&fifo_ptr->mtx);
    while (0 == fifo_ptr->avail_len) {
        pthread_cond_wait(&fifo_ptr->cv, &fifo_ptr->mtx);        
    }
    if (fifo_ptr->avail_len < read_size) { 
        read_size = fifo_ptr->avail_len;
    }
    for (count = 0; count < read_size; count++) {
        read_save_buf[count] = fifo_ptr->buffer_addr[fifo_ptr->rd_place];
        fifo_ptr->avail_len--;
        fifo_ptr->rd_place = (fifo_ptr->rd_place + 1) % fifo_ptr->buffer_size;
    }
    pthread_mutex_unlock(&fifo_ptr->mtx);
    pthread_cond_signal(&fifo_ptr->cv);

    return read_size;
}

/******************************************************************************************
 * Function Name :  fifo_write
 * Description   :  往fifo中写入字符内容，并返回写入的字符数
 * Parameters    :  fifo_ptr：fifo结构体指针
                    write_buf: 要写入的字符
                    write_size：要写入字符的大小
 * Returns       :  成功返回 写入成功的字符数
                    失败返回 -1
*******************************************************************************************/
int fifo_write(st_fifo_buf *fifo_ptr, uint8_t *write_buf, int write_size)
{
    int count = 0;
    int ret = 0;

    if (NULL == fifo_ptr || NULL == write_buf || write_size <= 0) {
        dbg_perror("fifo_read failed!\n");
        return -1;
    }
    
    pthread_mutex_lock(&fifo_ptr->mtx);
    while (0 == fifo_ptr->buffer_size - fifo_ptr->avail_len) { 
        pthread_cond_wait(&fifo_ptr->cv, &fifo_ptr->mtx);     
    }
    if (fifo_ptr->buffer_size - fifo_ptr->avail_len < write_size) { 
        write_size = fifo_ptr->buffer_size - fifo_ptr->avail_len;
    }
    for (count = 0; count < write_size; count++) {
        fifo_ptr->buffer_addr[fifo_ptr->wr_place] = write_buf[count];
        fifo_ptr->avail_len++;
        fifo_ptr->wr_place = (fifo_ptr->wr_place + 1) % fifo_ptr->buffer_size;
    }
    pthread_mutex_unlock(&fifo_ptr->mtx);
    pthread_cond_signal(&fifo_ptr->cv);

    return write_size;
}

/******************************************************************************************
 * Function Name :  fifo_read_byte
 * Description   :  从fifo中读取一个字节
 * Parameters    :  fifo_ptr：fifo结构体指针
 *                  read_save_buf：存放读取到的数据
 * Returns       :  成功返回 读取到的字符数
                    失败返回 -1
*******************************************************************************************/
int fifo_read_byte(st_fifo_buf *fifo_ptr, uint8_t *read_save_buf)
{
    if (NULL == fifo_ptr) {
        dbg_perror("fifo_read_byte failed!\n");
        return -1;
    }

    return fifo_read(fifo_ptr, read_save_buf, 1);
}

/******************************************************************************************
 * Function Name :  fifo_write_byte
 * Description   :  往fifo中写入一个字节
 * Parameters    :  fifo_ptr：fifo结构体指针
 *                  read_save_buf：待写入的数据
 * Returns       :  成功返回 写入成功的字符数
                    失败返回 -1
*******************************************************************************************/
int fifo_write_byte(st_fifo_buf *fifo_ptr, uint8_t write_buf)
{
    if (NULL == fifo_ptr) {
        dbg_perror("fifo_read_byte failed!\n");
        return -1;
    }

    return fifo_write(fifo_ptr, &write_buf, 1);
}

/******************************************************************************************
 * Function Name :  fifo_init
 * Description   :  fifo结构体初始化
 * Parameters    :  fifo_ptr：fifo结构体指针
                    fifo_size: fifo结构体大小
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int fifo_init(st_fifo_buf *fifo_ptr, int fifo_size)
{
    if (NULL == fifo_ptr || fifo_size <= 0) {
        dbg_perror("fifo_init failed!");
        return -1;
    }

    fifo_ptr->buffer_addr = (uint8_t *)malloc(sizeof(uint8_t) * fifo_size);
    if (NULL == fifo_ptr->buffer_addr) {
        dbg_perror("malloc fifo_ptr failed!");
        return -1;
    }

    fifo_ptr->avail_len = 0;
    fifo_ptr->buffer_size = fifo_size;
    fifo_ptr->wr_place = 0;           
    fifo_ptr->rd_place = 0; 

    pthread_mutex_init(&fifo_ptr->mtx, NULL);
    pthread_cond_init(&fifo_ptr->cv, NULL);

    return 0;
}

/******************************************************************************************
 * Function Name :  fifo_deinit
 * Description   :  释放fifo中的堆空间
 * Parameters    :  fifo_ptr：fifo结构体指针
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int fifo_deinit(st_fifo_buf *fifo_ptr)
{
    if (NULL == fifo_ptr) {
        dbg_perror("fifo_uninit failed!\n");
        return -1;
    }
    
    pthread_cond_destroy(&fifo_ptr->cv);
    pthread_mutex_destroy(&fifo_ptr->mtx);

    free(fifo_ptr->buffer_addr);
    free(fifo_ptr);

    return 0;
}

