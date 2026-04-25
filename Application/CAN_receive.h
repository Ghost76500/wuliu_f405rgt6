#ifndef CAN_RECEIVE_H
#define CAN_RECEIVE_H

#include "can.h"
#include "chassis_task.h"


// CAN1用于与OPS里程计通信
#define OPS_CAN hcan1
// CAN2用于控制底盘电机
#define CHASSIS_CAN hcan2

// 协议约定：yaw_raw 取值大约在 [-31415, +31415]，表示 [-3.1415, +3.1415] rad 的 1e4 放大。
// 如你的上位机/陀螺仪发送端使用不同缩放，请同步修改该比例。
#define CAN_GYRO_YAW_SCALE (0.0001f)
#define CAN_DISTENCE_SCALE (0.001f)
#define CHASSIS_MOTOR_RPM_LIMIT (200)

// 底盘四个电机方向修正：1为正向，-1为反向
#define CHASSIS_MOTOR1_DIR (1)
#define CHASSIS_MOTOR2_DIR (-1)
#define CHASSIS_MOTOR3_DIR (1)
#define CHASSIS_MOTOR4_DIR (-1)

// CAN线收发id枚举列表
typedef enum
{
  CAN_Y42_M1_ID = 0x01,     // 一号底盘电机id
  CAN_Y42_M2_ID = 0x02,     // 二号底盘电机id
  CAN_Y42_M3_ID = 0x03,     // 三号底盘电机id
  CAN_Y42_M4_ID = 0x04,     // 四号底盘电机id
  
  CAN_OPS_ID = 0x200,           // OPS里程计id
} can_msg_id_e;

// ZDT张大头电机数据结构体
typedef struct
{
  uint8_t motor_id;      // 电机ID
  fp32 speed_rpm;     // 电机转速
} motor_measure_t;

/**
 * @brief 向底盘的电机发送控制电流
 * @param[in] motor1 id为0x001的电机发送控制速度
 * @param[in] motor2 id为0x002
 * @param[in] motor3 id为0x003
 * @param[in] motor4 id为0x004
 * @retval none
 */
extern void can_send_chassis_speed(int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4);

extern void can_SendCmd(__IO uint8_t *cmd, uint8_t len);

extern void BSP_CAN_Send_Msg(uint32_t std_id, uint8_t *data);
extern volatile motor_measure_t can_y42_motor_measure[4];
extern volatile uint8_t can_y42_new_data_flag;
extern volatile float can_gyro_yaw_rad;
extern volatile float can_distence_x_m;
extern volatile float can_distence_y_m;
extern volatile uint8_t can_odom_new_data_flag;
extern void can_chassis_init(void); // 使能底盘can步进电机驱动板

#endif /* CAN_RECEIVE_H */