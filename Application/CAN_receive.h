#ifndef CAN_RECEIVE_H
#define CAN_RECEIVE_H

#include "stm32f4xx_hal.h"
#include "chassis_task.h"

extern void BSP_CAN_Send_Msg(uint32_t std_id, uint8_t *data);
extern volatile float can_gyro_yaw_rad;
extern volatile float can_distence_x_m;
extern volatile float can_distence_y_m;
extern volatile uint8_t can_odom_new_data_flag;

#endif /* CAN_RECEIVE_H */