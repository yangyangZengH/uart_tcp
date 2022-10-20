#include "uart_dev.h"

static struct termios oldtio;



static void uart_dev_init_data_bit(struct termios *newtio, int data_bit)
{
	switch (data_bit) {
		case 5:
			newtio->c_cflag |= CS5;
			break;
		case 6:
			newtio->c_cflag |= CS6;
			break;
		case 7:
			newtio->c_cflag |= CS7;
			break;
		case 8:
			newtio->c_cflag |= CS8;
			break;
		default:
			newtio->c_cflag |= CS8;
			break;
	}
}

static void uart_dev_init_check_sum(struct termios *newtio, int check_sum)
{
	switch (check_sum) {
		case 'o':
		case 'O': // 奇数
			newtio->c_cflag |= PARENB;
			newtio->c_cflag |= PARODD;
			newtio->c_iflag |= (INPCK | ISTRIP);
			break;
		case 'e':
		case 'E': // 偶数
			newtio->c_iflag |= (INPCK | ISTRIP);
			newtio->c_cflag |= PARENB;
			newtio->c_cflag &= ~PARODD;
			break;
		case 'n':
		case 'N': // 无奇偶校验位
			newtio->c_cflag &= ~PARENB;
			break;
	}
}

static void uart_dev_init_bps(struct termios *newtio, int bps)
{
	switch (bps) {
		case 9600:
			cfsetispeed(newtio, B9600);
			cfsetospeed(newtio, B9600);
			break;
		case 19200:
			cfsetispeed(newtio, B19200);
			cfsetospeed(newtio, B19200);
			break;
		case 38400:
			cfsetispeed(newtio, B38400);
			cfsetospeed(newtio, B38400);
			break;
		case 57600:
			cfsetispeed(newtio, B57600);
			cfsetospeed(newtio, B57600);
			break;
		case 115200:
			cfsetispeed(newtio, B115200);
			cfsetospeed(newtio, B115200);
			break;
		default:
			cfsetispeed(newtio, B9600);
			cfsetospeed(newtio, B9600);
			break;
	}
}

static void uart_dev_init_stop(struct termios *newtio, int stop)
{
	switch (stop) {
		case 1:
			newtio->c_cflag &= ~CSTOPB;
			break;
		case 2:
			newtio->c_cflag |= CSTOPB;
			break;
		default:
			newtio->c_cflag &= ~CSTOPB;
			break;
	}
}

/******************************************************************************************
 * Function Name :  uart_dev_init
 * Description   :  打开串口对应的设备节点，串口初始化，配置波特率，数据位，奇偶校验位和停止位
 * Parameters    :  
 * Returns       :  成功返回 fd
					失败返回 -1
*******************************************************************************************/
int uart_dev_init(st_uart_dev *uart_dev)
{
	struct termios newtio;

	uart_dev->uart_fd = open(UART_DEV, O_RDWR);
	if (-1 == uart_dev->uart_fd) {
	    dbg_perror("open device error.\n");
	    return -1;
	}
	/* 保存该串口原始数据 */
	memset(&oldtio, 0, sizeof(oldtio));
	if (tcgetattr(uart_dev->uart_fd, &oldtio) == 0) {
		dbg_printf("save old Serial!\n");
	}
	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;
	/* 设置数据位 */
	uart_dev_init_data_bit(&newtio, uart_dev->data_bit);
	/* 设置奇偶校验位 */
	uart_dev_init_check_sum(&newtio, uart_dev->check_sum);
	/* 设置波特率 */
	uart_dev_init_bps(&newtio, uart_dev->bps);
	/* 设置停止位 */
	uart_dev_init_stop(&newtio, uart_dev->stop);
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 1;
	tcflush(uart_dev->uart_fd, TCIFLUSH);
	/* 设置串口参数 */
	if ((tcsetattr(uart_dev->uart_fd, TCSANOW, &newtio)) != 0) {
		dbg_perror("set Serial error");
		if (tcsetattr(uart_dev->uart_fd, TCSANOW, &oldtio) == 0) {
			dbg_printf("recover old Serial!\n");
		}
		return -1;
	}
	dbg_printf("uart set done! fd:%d \n", uart_dev->uart_fd);

	return uart_dev->uart_fd;
}

/******************************************************************************************
 * Function Name :  uart_dev_read
 * Description   :  读取串口中的内容，并返回读取到的字符数
 * Parameters    :  fd：串口设备的文件描述符
 					data_buf: 存放读取到的内容的空间
 					len: 要读取的字符长度
 * Returns       :  成功 返回读取的字符数
 					失败 -1
 *******************************************************************************************/
int uart_dev_read(int fd, uint8_t *data_buf, int len)
{
	int ret = 0;

	if (NULL == data_buf) {
		dbg_perror("uart_read arg error!");
		return -1;
	} 
	ret = read(fd, data_buf, len);
	if (ret < 0) {
		dbg_perror("uart_read failed!");
		return -1;
	}

	return ret;
}

/******************************************************************************************
 * Function Name :  uart_dev_write
 * Description   :  往串口中写入字符内容，并返回写入的字符数
 * Parameters    :  fd：串口设备的文件描述符
 					data_buf: 要写入的字符
 					len: 要写入的字符长度
 * Returns       :  成功 返回写入的字符数
 					失败 -1
 *******************************************************************************************/
int uart_dev_write(int fd, uint8_t *data_buf, int len)
{
	int ret = 0;

	if (NULL == data_buf) {
		dbg_perror("uart_write arg error!");
		return -1;
	}
	ret = write(fd, data_buf, len);
	if (ret < 0) {
		dbg_perror("uart_write failed!");
		return -1;
	}

	return ret;
}

/******************************************************************************************
 * Function Name :  uart_dev_deinit
 * Description   :  串口去初始化，关闭串口对应的设备节点
 * Parameters    :  fd：串口设备的文件描述符
 * Returns       :  成功 0
 					失败 -1
 *******************************************************************************************/
int uart_dev_deinit(st_uart_dev *uart_dev)
{
	int ret = 0;

	if (tcsetattr(uart_dev->uart_fd, TCSANOW, &oldtio) == 0) {
		dbg_printf("recover old Serial!\n");
	}
	
	ret = close(uart_dev->uart_fd);
	if (-1 == ret) {
		dbg_perror("uart_dev_deinit failed");
		return -1;
	}

	uart_dev->uart_fd = -1;

	return 0;
}
