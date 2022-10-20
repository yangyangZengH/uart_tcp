#include "hip.h"


typedef union un_drop_frame{
    float frame;
    uint8_t store[4];
}un_drop_frame;




static st_hip_payload payload = {0};
static un_drop_frame drop_frame = {0};

/******************************************************************************************
 * Function Name :  hip_pack_payload
 * Description   :  根据命令类型，填充hip负载数据
 * Parameters    :  payload：存放负载数据的st_hip_payload结构体指针
 *                  cmd：hip负载数据的命令类型
 *                  length：hip负载数据的长度
 *                  data：hip的负载数据
 * Returns       :  成功返回 0
 *                  失败返回 -1
*******************************************************************************************/
int hip_pack_payload(st_hip_payload *payload, uint8_t cmd, uint16_t length, uint8_t *data, float send_frame, float recv_frame)
{
    int count = 0;

    if (NULL == payload) {
        dbg_perror("hip_pack_payload arg error.");
        return -1;
    }

    payload->dev_type  = cmd;
    switch (cmd) {
        case EM_HIP_USER_LOGIN:
        case EM_HIP_KEEP_ALIVE:
            // mac = 00:0c:29:ed:7b:e3
            payload->mac[0] = 0x00;
            payload->mac[1] = 0x0c;
            payload->mac[2] = 0x29;
            payload->mac[3] = 0xed;
            payload->mac[4] = 0x7b;
            payload->mac[5] = 0xe3;
            // ip = 127.0.0.1
            payload->ip[0] = 127;
            payload->ip[1] = 0;
            payload->ip[2] = 0;
            payload->ip[3] = 1;
            switch (cmd) {
                case EM_HIP_USER_LOGIN:
                    payload->login_state = 0;
                    break;
                case EM_HIP_KEEP_ALIVE:
                    drop_frame.frame = recv_frame / send_frame;
                    payload->lostframe[0] = drop_frame.store[3];
                    payload->lostframe[1] = drop_frame.store[2];
                    payload->lostframe[2] = drop_frame.store[1];
                    payload->lostframe[3] = drop_frame.store[0];
                    break;
            }
            break;
        case EM_HIP_UART_PASS_THROUGH:
            payload->data_length[0] = (length >> 8);
            payload->data_length[1] = length & 0xff;
            for (count =0; count < length; count++) {
                payload->uart_data[count] = data[count];
            }
            break;
        default :
            dbg_perror("hip_pack_payload switch default! \n");
            break;
    }
    return 0;
}

/******************************************************************************************
 * Function Name :  hip_pack_user_login_data
 * Description   :  打包用户登录数据
 * Parameters    :  pack_data：数据打包后存放区域
 *                  hip_payload：待打包的负载数据
 * Returns       :  无
*******************************************************************************************/
static void hip_pack_user_login_data(uint8_t *pack_data, st_hip_payload *hip_payload)
{
    if (NULL == hip_payload || NULL == pack_data) {
		dbg_perror("hip_pack_user_login_data arg error!");
		return ;
	} 
    pack_data[16] = hip_payload->dev_type;
    pack_data[17] = hip_payload->mac[0];
    pack_data[18] = hip_payload->mac[1];
    pack_data[19] = hip_payload->mac[2];
    pack_data[20] = hip_payload->mac[3];
    pack_data[21] = hip_payload->mac[4];
    pack_data[22] = hip_payload->mac[5];
    pack_data[23] = hip_payload->ip[0];
    pack_data[24] = hip_payload->ip[1];
    pack_data[25] = hip_payload->ip[2];
    pack_data[26] = hip_payload->ip[3];
    pack_data[27] = hip_payload->login_state;
    pack_data[2]  = HIP_HEAD_LENGTH + HIP_USER_LOGIN_LENGTH;
}

/******************************************************************************************
 * Function Name :  hip_pack_keep_alive_data
 * Description   :  打包心跳保持数据
 * Parameters    :  pack_data：数据打包后存放区域
 *                  hip_payload：待打包的负载数据
 * Returns       :  无
*******************************************************************************************/
static void hip_pack_keep_alive_data(uint8_t *pack_data, st_hip_payload *hip_payload)
{
    if (NULL == hip_payload || NULL == pack_data) {
		dbg_perror("hip_pack_keep_alive_data arg error!");
		return ;
	} 
    pack_data[16] = hip_payload->dev_type;
    pack_data[17] = hip_payload->mac[0];
    pack_data[18] = hip_payload->mac[1];
    pack_data[19] = hip_payload->mac[2];
    pack_data[20] = hip_payload->mac[3];
    pack_data[21] = hip_payload->mac[4];
    pack_data[22] = hip_payload->mac[5];
    pack_data[23] = hip_payload->ip[0];
    pack_data[24] = hip_payload->ip[1];
    pack_data[25] = hip_payload->ip[2];
    pack_data[26] = hip_payload->ip[3];
    pack_data[27] = hip_payload->lostframe[0];
    pack_data[28] = hip_payload->lostframe[1];
    pack_data[29] = hip_payload->lostframe[2];
    pack_data[30] = hip_payload->lostframe[3];
    pack_data[2]  = HIP_HEAD_LENGTH + HIP_KEEP_ALIVE_LENGTH;
}

/******************************************************************************************
 * Function Name :  hip_pack_uart_through_data
 * Description   :  打包心跳保持数据
 * Parameters    :  pack_data：数据打包后存放区域
 *                  hip_payload：待打包的负载数据
 * Returns       :  无
*******************************************************************************************/
static void hip_pack_uart_through_data(uint8_t *pack_data, st_hip_payload *hip_payload)
{
    int count = 0;
    int data_length = 0;

    if (NULL == hip_payload || NULL == pack_data) {
		dbg_perror("hip_pack_uart_through_data arg error!");
		return ;
	} 
    pack_data[16] = hip_payload->data_length[1];
    pack_data[17] = hip_payload->data_length[2];
    data_length = (pack_data[16] << 8) | pack_data[17];
    for (count = 0; count < data_length; count++) {
        pack_data[18+count] = hip_payload->uart_data[count];
    }
    pack_data[2]  = HIP_HEAD_LENGTH + HIP_UART_THROUGH_LENGTH + count;
    free(hip_payload->uart_data);
}

/******************************************************************************************
 * Function Name :  hip_pack
 * Description   :  对数据进行hip打包
 * Parameters    :  cmd：待打包数据的命令类型
 *                  hip_payload：hip中负载数据的结构体指针
 *                  pack_data：打包完成后存放的区域
 * Returns       :  成功：整个帧的数据长度
 *                  失败：-1
*******************************************************************************************/
int hip_pack(uint8_t cmd, st_hip_payload *hip_payload, uint8_t *pack_data)
{
    static uint32_t hip_seq = 0;
    static uint64_t hip_dev_id = 0xc29ed7be3;

    if (NULL == hip_payload || NULL == pack_data) {
		dbg_perror("hip_pack arg error!");
		return -1;
	} 

    hip_seq++;
    memset(pack_data, 0, sizeof(pack_data));
    pack_data[0]  = HIP_VERSION;              
    pack_data[1]  = cmd;             
    pack_data[2]  = HIP_HEAD_LENGTH;
    pack_data[3]  = HIP_DEV_TYPE;       
    pack_data[4]  = (hip_dev_id >> 56) & 0xff;   
    pack_data[5]  = (hip_dev_id >> 48) & 0xff;
    pack_data[6]  = (hip_dev_id >> 40) & 0xff;
    pack_data[7]  = (hip_dev_id >> 32) & 0xff;
    pack_data[8]  = (hip_dev_id >> 24) & 0xff;
    pack_data[9]  = (hip_dev_id >> 16) & 0xff;
    pack_data[10] = (hip_dev_id >>  8) & 0xff;
    pack_data[11] = (hip_dev_id >>  0) & 0xff;
    pack_data[12] = (hip_seq >> 24) & 0xff;
    pack_data[13] = (hip_seq >> 16) & 0xff;
    pack_data[14] = (hip_seq >>  8) & 0xff;
    pack_data[15] = (hip_seq >>  0) & 0xff;

    switch (cmd) {
        case EM_HIP_USER_LOGIN:
            hip_pack_user_login_data(pack_data, hip_payload);
            break;
        case EM_HIP_KEEP_ALIVE:
            hip_pack_keep_alive_data(pack_data, hip_payload);
            break;
        case EM_HIP_UART_PASS_THROUGH:
            hip_pack_uart_through_data(pack_data, hip_payload);
            break;
        default:
            dbg_perror("hip_pack switch default! \n");
            break;
    }
    printf("pack_data[2]:%d\n",pack_data[2]);
    for (int i = 0; i < pack_data[2]; i++) {
        printf("%d ",pack_data[i]);
    }
    printf("\n");
    return 0;
}

//----------------------------------------------------------------------------------------------------

/******************************************************************************************
 * Function Name :  hip_depack_user_login_data
 * Description   :  解包用户登录数据
 * Parameters    :  pack_data：待解析的数据
 *                  hip_payload：st_hip_payload结构体指针, 存放解析后的负载数据
 * Returns       :  无
*******************************************************************************************/
static void hip_depack_user_login_data(uint8_t pack_data, st_hip_payload *hip_payload)
{
    static int count = 0;
    static int s_payload_state = EM_PAYLOAD_DEVICE_TYPE;

    switch (s_payload_state) {
        case EM_PAYLOAD_DEVICE_TYPE:
            hip_payload->dev_type = pack_data;
            s_payload_state = EM_PAYLOAD_MAC;
            break;
        case EM_PAYLOAD_MAC:
            hip_payload->mac[count++] = pack_data;
            if (MAC_SIZE == count) {
                count = 0;
                s_payload_state = EM_PAYLOAD_IP;
            }
            break;
        case EM_PAYLOAD_IP:
            hip_payload->ip[count++] = pack_data;
            if (IP_SIZE == count) {
                count = 0;
                s_payload_state = EM_PAYLOAD_LOGIN_STATE;
                hip_payload->depack_status  = true;
            }
            break;
        case EM_PAYLOAD_LOGIN_STATE:
            hip_payload->login_state = pack_data;
            s_payload_state = EM_PAYLOAD_DEVICE_TYPE;
            hip_payload->depack_status  = true;
            break;     
        default:
            dbg_perror("hip_depack_user_login_data switch default! \n");
            break;
    }
}

/******************************************************************************************
 * Function Name :  hip_depack_keep_alive_data
 * Description   :  解包心跳保持数据
 * Parameters    :  pack_data：待解析的数据
 *                  hip_payload：st_hip_payload结构体指针, 存放解析后的负载数据
 * Returns       :  无
*******************************************************************************************/
static void hip_depack_keep_alive_data(uint8_t pack_data, st_hip_payload *hip_payload)
{
    static int count = 0;
    static int s_payload_state = EM_PAYLOAD_DEVICE_TYPE;

    switch (s_payload_state) {
        case EM_PAYLOAD_DEVICE_TYPE:
            hip_payload->dev_type = pack_data;
            s_payload_state = EM_PAYLOAD_MAC;
            break;
        case EM_PAYLOAD_MAC:
            hip_payload->mac[count++] = pack_data;
            if (MAC_SIZE == count) {
                count = 0;
                s_payload_state = EM_PAYLOAD_IP;
            }
            break;
        case EM_PAYLOAD_IP:
            hip_payload->ip[count++] = pack_data;
            if (IP_SIZE == count) {
                count = 0;
                s_payload_state = LOSTFRAME_SIZE;
            }
            break;
        case LOSTFRAME_SIZE:
            hip_payload->lostframe[count++] = pack_data;
            if (LOSTFRAME_SIZE == count) {
                count = 0;
                s_payload_state = EM_PAYLOAD_DEVICE_TYPE;
                hip_payload->depack_status  = true;
            }
            break;
        default:
            dbg_perror("hip_depack_user_login_data switch default! \n");
            break;
    }
}

/******************************************************************************************
 * Function Name :  hip_depack_uart_through_data
 * Description   :  解包串口透传数据
 * Parameters    :  pack_data：待解析的数据
 *                  hip_payload：st_hip_payload结构体指针, 存放解析后的负载数据
 * Returns       :  无
*******************************************************************************************/
static void hip_depack_uart_through_data(uint8_t pack_data, st_hip_payload *hip_payload)
{
    static int length = 0;
    static int count = 0;
    static int s_payload_state = EM_PAYLOAD_DATA_LENGTH_H;

    switch (s_payload_state) {
        case EM_PAYLOAD_DATA_LENGTH_H:
            hip_payload->data_length[0] = pack_data;
            s_payload_state = EM_PAYLOAD_DATA_LENGTH_L;
            break;
        case EM_PAYLOAD_DATA_LENGTH_L:
            hip_payload->data_length[1] = pack_data;
            s_payload_state = EM_PAYLOAD_UART_DATA;
            break;
        case EM_PAYLOAD_UART_DATA:
            length = (hip_payload->data_length[0] << 8) | hip_payload->data_length[1];
            hip_payload->uart_data[count++] = pack_data;
            if (length == count) {
                count = 0;
                s_payload_state = EM_PAYLOAD_DATA_LENGTH_H;
                hip_payload->depack_status  = true;
            }
            break;
        default:
            dbg_perror("hip_depack_user_login_data switch default! \n");
            break;
    }
}

/******************************************************************************************
 * Function Name :  hip_depack_payload
 * Description   :  根据cmd解析不同负载数据
 * Parameters    :  pack_data：待解析的数据
 *                  hip_pack：hip结构体指针, 存放解析成功后的有用数据
 * Returns       :  无
*******************************************************************************************/
static void hip_depack_payload(uint8_t pack_data, st_hip_pack *hip_pack)
{
    switch (hip_pack->commond) {
        case EM_HIP_USER_LOGIN:
            hip_depack_user_login_data(pack_data, &hip_pack->hip_payload);
            break;
        case EM_HIP_KEEP_ALIVE:
            hip_depack_keep_alive_data(pack_data, &hip_pack->hip_payload);
            break;
        case EM_HIP_UART_PASS_THROUGH:
            hip_depack_uart_through_data(pack_data, &hip_pack->hip_payload);
            break;
        default:
            dbg_perror("hup_unpack switch default! \n");
            break;
    }
}

/******************************************************************************************
 * Function Name :  hip_depack
 * Description   :  根据传入的pack_data，进行hip协议解析包数据
 * Parameters    :  pack_data：待解析的数据
 *                  hip_pack：hip结构体指针, 存放解析成功后的有用数据
 * Returns       :  成功返回 0
 *                  失败返回 -1
*******************************************************************************************/
int hip_depack(uint8_t pack_data, st_hip_pack *hip_pack)
{
    static int s_state = EM_HIP_VERSION;
    static int count = 0;

    if (NULL == hip_pack) {
        dbg_perror("hip_depack error!");
        return -1;
    }
    hip_pack->hip_payload.depack_status = false;
    switch (s_state) {
        case EM_HIP_VERSION:
            memset(hip_pack, 0, sizeof(hip_pack));
            hip_pack->version = pack_data;
            s_state = EM_HIP_COMMOND;
            break;
        case EM_HIP_COMMOND:
            hip_pack->commond = pack_data;
            s_state = EM_HIP_LENGTH;
            break;
        case EM_HIP_LENGTH:
            hip_pack->length = pack_data;
            s_state = EM_HIP_DEVICE_TYPE;
            break;
         case EM_HIP_DEVICE_TYPE:
            hip_pack->dev_type = pack_data;
            s_state = EM_HIP_DEVICE_ID;
            break;
         case EM_HIP_DEVICE_ID:
            hip_pack->dev_id[count++] = pack_data;
            if (DEV_ID_SIZE == count) {
                count = 0;
                s_state = EM_HIP_SEQ;
            }
            break;
         case EM_HIP_SEQ:
            hip_pack->seq[count++] = pack_data;
            if (SEQ_SIZE == count) {
                count = 0;
                s_state = EM_HIP_PAYLOAD;
            }
            break;
         case EM_HIP_PAYLOAD:
            hip_depack_payload(pack_data, hip_pack);
            if (hip_pack->hip_payload.depack_status) {
                s_state = EM_HIP_VERSION;
            }
            break;
        default :
            dbg_perror("hup_unpack switch default! \n");
            break;
    }

    return 0;
}
