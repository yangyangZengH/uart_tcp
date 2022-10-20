#include "hup.h"


/******************************************************************************************
 * 帧头标识符
 * 请求标识符帧头为0xAA 0xDD
 * 响应标识符帧头为0xBB 0xDD
*******************************************************************************************/
#define HUP_TAG_HA  0xAA    
#define HUP_TAG_LD  0xDD
#define HUP_TAG_HB  0xBB

enum HUP_STATE 
{
    HUP_TAG_ONE = 1,
    HUP_TAG_TWO,
    HUP_CMD_ID,
    HUP_LENGTH_H,
    HUP_LENGTH_L,
    HUP_DATA,
    HUP_CHECK_SUM
};

/******************************************************************************************
 * Function Name :  hup_depack
 * Description   :  根据传入的pack_data，进行hup协议解析包数据
 * Parameters    :  pack_data：待解析的包数据
                    msg: 存放解析成功后的有用数据
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int hup_depack(uint8_t pack_data, st_pack_msg *msg)
{
    static int s_data_num = 0;
    static uint8_t s_ckeck_sum = 0;
    static int s_state = HUP_TAG_ONE;

    if (NULL == msg) {
        dbg_perror("hup_unpack failed!\n");
        return -1;
    }
    msg->depack_status = false;
    //msg->length_status = false;
    switch (s_state) {
        case HUP_TAG_ONE:
            if (HUP_TAG_HA == pack_data) { 
                s_data_num = 0;
                s_state = HUP_TAG_TWO;
                s_ckeck_sum ^= pack_data;
            } else {
                s_state = HUP_TAG_ONE;
            }
            break;
        case HUP_TAG_TWO:
            if (HUP_TAG_LD == pack_data) { 
                s_state = HUP_CMD_ID;
                s_ckeck_sum ^= pack_data;
            } else {
                s_state = HUP_TAG_ONE;
            }
            break;
        case HUP_CMD_ID:
            msg->cmd_id = pack_data;
            s_state = HUP_LENGTH_H;
            s_ckeck_sum ^= pack_data;
            break;
        case HUP_LENGTH_H:
            msg->length = pack_data;
            s_state = HUP_LENGTH_L;
            break;
        case HUP_LENGTH_L:
            msg->length = (msg->length << 8) | pack_data;
            s_ckeck_sum ^= msg->length;
            if (0 == msg->length) {
                s_state = HUP_CHECK_SUM;
            } else {
                s_state = HUP_DATA;
                msg->length_status = true;
            }
            break;
        case HUP_DATA:
            s_ckeck_sum ^= pack_data;
            msg->data_buf[s_data_num] = pack_data;
            s_data_num++;
            if (s_data_num == msg->length) {
                s_state = HUP_CHECK_SUM;
                msg->length_status = false;
            }
            break;
        case HUP_CHECK_SUM:
            dbg_printf("------s_ckeck_sum:%x------\n", s_ckeck_sum);
            if (s_ckeck_sum == pack_data) {
                msg->depack_status = true;
            }
            s_state = HUP_TAG_ONE;
            s_ckeck_sum = 0;           
            break;
        default :
            dbg_perror("hup_unpack switch default! \n");
            break;
    }

    return 0;
}

/******************************************************************************************
* Function Name :  hup_pack
* Description   :  根据传入的msg，进行hup协议打包数据,存入pack_data
* Parameters    :  msg: 待打包的数据
                   data_buf：hup打包完成的数据
* Returns       :  成功返回 0
                   失败返回 -1
*******************************************************************************************/
int hup_pack(st_pack_msg *msg, uint8_t *pack_data)
{
    int count = 0;
    uint8_t check_sum = 0;

	if (NULL == msg || NULL == pack_data) {
		dbg_perror("hup_pack arg error!");
		return -1;
	} 

    pack_data[0] = HUP_TAG_HB;              
    pack_data[1] = HUP_TAG_LD;             
    pack_data[2] = msg->cmd_id;
    pack_data[3] = msg->length >> 8;       
    pack_data[4] = msg->length & 0xff;    
    check_sum = pack_data[0]^pack_data[1]^pack_data[2]^pack_data[3]^pack_data[4];
    for (count = 0; count < msg->length; count++) {
        pack_data[count+5] = msg->data_buf[count];
        check_sum ^= msg->data_buf[count];
    }
    pack_data[count+5] = check_sum;

    return 0;
}

