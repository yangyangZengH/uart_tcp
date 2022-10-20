#ifndef __HUP_H__
#define __HUP_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "common.h"

typedef struct st_pack_msg
{
    uint8_t cmd_id;
    uint16_t length;
    uint8_t data_buf[128];
    bool depack_status;
    bool length_status;
}st_pack_msg;


/******************************************************************************************
 * Function Name :  hup_depack
 * Description   :  根据传入的pack_data，进行hup协议解析包数据
 * Parameters    :  pack_data：待解析的包数据
                    msg: 存放解析成功后的有用数据
 * Returns       :  成功返回 0
                    失败返回 -1
*******************************************************************************************/
int hup_depack(uint8_t pack_data, st_pack_msg *msg);

/******************************************************************************************
* Function Name :  hup_pack
* Description   :  根据传入的msg，进行hup协议打包数据,存入pack_data
* Parameters    :  msg: 待打包的数据
                   data_buf：hup打包完成的数据
* Returns       :  成功返回 0
                   失败返回 -1
*******************************************************************************************/
int hup_pack(st_pack_msg *msg, uint8_t *pack_data);




#endif  //__HUP_H__

