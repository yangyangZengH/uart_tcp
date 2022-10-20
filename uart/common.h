/******************************************************************************
* common.h
*******************************************************************************/

#ifndef __COMMON_H__


#define DEBUG 1

#ifdef DEBUG
#define dbg_printf(fmt, args...) printf(fmt, ##args)
#define dbg_perror(msg) (perror(msg))
#else
#define dbg_printf(fmt, args...)
#define dbg_perror(msg)
#endif

#define MIN(x,y)  (((x)<(y))?(x):(y))
#define MAX(x,y)  (((x)>(y))?(x):(y))

#define UART_BPS        115200
#define UART_DATA_BIT   8
#define UART_CHECK_SUM  'N'
#define UART_STOP       1

#define FIFO_SIZE       255

#endif		// __COMMON_H__

