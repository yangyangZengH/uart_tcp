#ifndef __HIP_H__
#define __HIP_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

#define DEV_ID_SIZE             8
#define SEQ_SIZE                4
#define MAC_SIZE                6
#define IP_SIZE                 4
#define LOSTFRAME_SIZE          4
#define DATA_LENGTH             2
#define UART_DATA_SIZE          255

#define HIP_VERSION              1
#define HIP_DEV_TYPE             1
#define HIP_HEAD_LENGTH          16
#define HIP_USER_LOGIN_LENGTH    12
#define HIP_KEEP_ALIVE_LENGTH    15
#define HIP_UART_THROUGH_LENGTH  2


typedef struct st_hip_payload { 
    uint8_t dev_type;                   // 设备类型
    uint8_t mac[MAC_SIZE];              // MAC地址
    uint8_t ip[IP_SIZE];                // IP地址
    uint8_t lostframe[LOSTFRAME_SIZE];  // 丢帧率
    uint8_t login_state;                // 登录状态
    uint8_t data_length[DATA_LENGTH];   // 串口数据长度
    uint8_t uart_data[UART_DATA_SIZE];  // 串口数据
    bool depack_status;                 // 解包状态
}st_hip_payload;

typedef struct st_hip_pack {
    uint8_t version;                    // 版本号
    uint8_t commond;                    // 命令类型
    uint8_t length;                     // 报文长度
    uint8_t dev_type;                   // 设备类型
    uint8_t dev_id[DEV_ID_SIZE];        // 设备ID
    uint8_t seq[SEQ_SIZE];              // 命令序列号
    st_hip_payload hip_payload;         // 载荷扩展字段结构体
}st_hip_pack;

/* 解析hip的状态机 */
typedef enum em_hip_pack_state{     
    EM_HIP_VERSION = 1,
    EM_HIP_COMMOND,
    EM_HIP_LENGTH,
    EM_HIP_DEVICE_TYPE,
    EM_HIP_DEVICE_ID,
    EM_HIP_SEQ,
    EM_HIP_PAYLOAD,
}em_hip_pack_state;     

/* 解析hip中payload的状态机 */
typedef enum em_hip_payload_state{  
    EM_PAYLOAD_DEVICE_TYPE = 9,             
    EM_PAYLOAD_MAC,
    EM_PAYLOAD_IP,
    EM_PAYLOAD_LOGIN_STATE,
    EM_PAYLOAD_LOSTFRAME,
    EM_PAYLOAD_DATA_LENGTH_H,
    EM_PAYLOAD_DATA_LENGTH_L,
    EM_PAYLOAD_UART_DATA,                  
}em_hip_payload_state;  

typedef enum em_hip_pack_cmd{
    EM_HIP_USER_LOGIN         = 0x02,     // 用户登录
    EM_HIP_KEEP_ALIVE         = 0x03,     // 心跳保持
    EM_HIP_UART_PASS_THROUGH  = 0x04,     // 串口透传
}em_hip_pack_cmd;



/******************************************************************************************
 * Function Name :  hip_pack
 * Description   :  对数据进行hip打包
 * Parameters    :  cmd：待打包数据的命令类型
 *                  hip_payload：hip中负载数据的结构体指针
 *                  pack_data：打包完成后存放的区域
 * Returns       :  成功：整个帧的数据长度
 *                  失败：-1
*******************************************************************************************/
int hip_pack(uint8_t cmd, st_hip_payload *hip_payload, uint8_t *pack_data);

/******************************************************************************************
 * Function Name :  hip_depack
 * Description   :  根据传入的pack_data，进行hip协议解析包数据
 * Parameters    :  pack_data：待解析的数据包
                    hip_pack：hip结构体指针, 存放解析成功后的有用数据
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int hip_depack(uint8_t pack_data, st_hip_pack *hip_pack);

/******************************************************************************************
 * Function Name :  hip_pack_payload
 * Description   :  根据命令类型，填充hip负载数据
 * Parameters    :  payload：存放负载数据的st_hip_payload结构体指针
 *                  cmd：hip负载数据的命令类型
 *                  length：hip负载数据的长度
 *                  data：hip的负载数据
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int hip_pack_payload(st_hip_payload *payload, uint8_t cmd, uint16_t length, uint8_t *data, float send_frame, float recv_frame);

#endif