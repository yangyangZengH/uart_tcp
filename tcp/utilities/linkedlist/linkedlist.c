#include "linkedlist.h"


/******************************************************************************************
 * Function Name :  create_linkedlist
 * Description   :  创建一个带头结点的空链表
 * Parameters    :  无
 * Returns       :  成功：st_linkedlist的结构体指针
 *					失败：NULL
*******************************************************************************************/
st_linkedlist *create_linkedlist()
{
    st_linkedlist *l = malloc(sizeof(*l));
	if (l == NULL) {
		dbg_perror("create_linkedlist error.");
		return NULL;
	}
	l->first = NULL;
	l->last = NULL;
	
	return l;
}

/******************************************************************************************
 * Function Name :  insert_node
 * Description   :  在链表的尾部插入一个结点
 * Parameters    :  l：st_linkedlist的结构体指针
 * 					p：待插入的数据结点
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
int insert_node(st_linkedlist *l, st_node *p)
{
    if (NULL == l || NULL == p) {
        dbg_perror("insert_node arg error .");
        return -1;
    }
    
    if (NULL == l->first) {
		l->first = p;
		l->last = p;

		return 0;
	}

    l->last->next = p;
    l->last = p;

    return 0;
}

/******************************************************************************************
 * Function Name :  delete_node
 * Description   :  删除链表中sockfd所在的结点
 * Parameters    :  l：st_linkedlist的结构体指针
 * 					sockfd：待删除的数据结点
 * Returns       :  成功：0
 *					失败：-1
*******************************************************************************************/
void delete_node(st_linkedlist *l, int sockfd)
{
	st_node *pp = l->first;
	st_node *pd = l->first;

    if (l == NULL || l->first == NULL) {
		return ;
	}

	while (pd != NULL) {
		if (pd->data.sockfd == sockfd) {
			pp->next = pd->next;
			pd->next = NULL;
			pd->data.sockfd = 0;
			free(pd);
		} else {
			pp = pd;
			pd = pd->next;
		}
	} 
}

/******************************************************************************************
 * Function Name :  destroy_linkedlist
 * Description   :  销毁该链表
 * Parameters    :  l：st_linkedlist的结构体指针
 * Returns       :  无
*******************************************************************************************/
void destroy_linkedlist(st_linkedlist* l)
{
	st_node *p = l->first;

	if (l == NULL)
		return;
	
	while (p)
	{
		st_node *px = p->next;
		p->next = NULL;
		free(p);
		p = px;
	}
	free(l);
}

/******************************************************************************************
 * Function Name :  get_node_num
 * Description   :  获取该链表的结点数量
 * Parameters    :  l：st_linkedlist的结构体指针
 * Returns       :  成功：当前链表的结点数量
 *					失败：-1
*******************************************************************************************/
int get_node_num(st_linkedlist* l)
{
	int count = 0;
	st_node *p = l->first;

	if (NULL == l) {
        dbg_perror("get_node_num arg error .");
        return -1;
    }

	while (p) {
		p = p->next;
		count++;
	}

	return count;
}

/******************************************************************************************
 * Function Name :  get_sockfd_node
 * Description   :  获取sockfd所在的结点
 * Parameters    :  l：st_linkedlist的结构体指针
 * 					sockfd：匹配链表中sockfd所在的结点
 * Returns       :  成功：返回sockfd所在的结点
 *					失败：NULL
*******************************************************************************************/
st_node *get_sockfd_node(st_linkedlist* l, int sockfd)
{
	st_node *p = l->first;

	if (NULL == l) {
        dbg_perror("get_sockfd_node arg error .");
        return NULL;
    }

	while (p) {
		if (p->data.sockfd == sockfd) {
			break;
		}
		p = p->next;
	}

	return p;
}

/******************************************************************************************
 * Function Name :  traversal_list
 * Description   :  遍历链表，sockfd存在返回0，不存在返回-1
 * Parameters    :  l：st_linkedlist的结构体指针
 * 					ip_addr：匹配链表中该ip是否存在
 * Returns       :  成功：0存在
 * 					失败：-1不存在
*******************************************************************************************/
int traversal_list(st_linkedlist* l, char *ip_addr)
{
	st_node *p = l->first;
	int flag = -1;

	if (NULL == l || ip_addr == NULL) {
        dbg_perror("traversal_list arg error .");
        return -1;
    }

	while (p) {
		if (strcmp(p->data.client_ip, ip_addr) == 0) {
			flag = 0;
			break;
		}
		p = p->next;
	}

	return flag;
}