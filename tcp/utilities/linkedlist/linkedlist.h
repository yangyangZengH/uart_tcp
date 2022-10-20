
#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#include <stdlib.h>
#include <stdio.h>
#include "common.h"


typedef struct st_tcp_client_info {
	int sockfd;					// 套接字
	int client_port;			// 端口
	char client_ip[32];			// ip地址
    time_t recv_time;			// 最后一次接收该客户端的时间
}st_tcp_client_info;

	
typedef struct st_node {
	st_tcp_client_info data; 	// 数据域
	struct st_node *next; 		// 指针域
}st_node;


typedef struct st_linkedlist {
	st_node *first; 			// 头结点
	st_node *last; 				// 尾结点
}st_linkedlist;



/******************************************************************************************
 * Function Name :  create_linkedlist
 * Description   :  创建一个带头结点的空链表
 * Parameters    :  无
 * Returns       :  成功：st_linkedlist的结构体指针
 *					失败：NULL
*******************************************************************************************/
st_linkedlist* create_linkedlist();

/******************************************************************************************
 * Function Name :  insert_node
 * Description   :  在链表的尾部插入一个结点
 * Parameters    :  l：st_linkedlist的结构体指针
 * 					p：待插入的数据结点
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int insert_node(st_linkedlist *l, st_node*p);

/******************************************************************************************
 * Function Name :  delete_node
 * Description   :  删除链表中sockfd所在的结点
 * Parameters    :  l：st_linkedlist的结构体指针
 * 					sockfd：待删除的数据结点
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
void delete_node(st_linkedlist *l, int sockfd);

/******************************************************************************************
 * Function Name :  destroy_linkedlist
 * Description   :  销毁该链表
 * Parameters    :  l：st_linkedlist的结构体指针
 * Returns       :  无
*******************************************************************************************/
void destroy_linkedlist(st_linkedlist* l);

/******************************************************************************************
 * Function Name :  get_node_num
 * Description   :  获取该链表的结点数量
 * Parameters    :  l：st_linkedlist的结构体指针
 * Returns       :  成功：当前链表的结点数量
 *					失败：-1
*******************************************************************************************/
int get_node_num(st_linkedlist* l);

/******************************************************************************************
 * Function Name :  get_sockfd_node
 * Description   :  获取sockfd所在的结点
 * Parameters    :  l：st_linkedlist的结构体指针
 * 					sockfd：匹配链表中sockfd所在的结点
 * Returns       :  成功：返回sockfd所在的结点
 *					失败：NULL
*******************************************************************************************/
st_node *get_sockfd_node(st_linkedlist* l, int sockfd);

/******************************************************************************************
 * Function Name :  traversal_list
 * Description   :  遍历链表，sockfd存在返回0，不存在返回-1
 * Parameters    :  l：st_linkedlist的结构体指针
 * 					ip_addr：匹配链表中该ip是否存在
 * Returns       :  成功：0存在
 * 					失败：-1不存在
*******************************************************************************************/
int traversal_list(st_linkedlist* l, char *ip_addr);


#endif
