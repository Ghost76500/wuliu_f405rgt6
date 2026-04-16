#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include "tim.h"
#include "stdint.h"
#include <stdint.h>


/*-----------------------------------数据结构定义-----------------------------------*/

typedef struct
{
    int16_t speed;
} encoder_motor_t;




/*-----------------------------------外部函数声明-----------------------------------*/

extern void bsp_encoder_init(void);
extern void encoder_update_all(void);
extern int16_t encoder_get_speed(uint8_t motor_id);


#endif