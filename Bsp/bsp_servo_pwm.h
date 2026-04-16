#ifndef __BSP_SERVO_PWM_H
#define __BSP_SERVO_PWM_H

#include "stdint.h"

/*云台指定角度宏定义*/
#define GIMBAL_MIN_ANGLE 500   // 最小占空比，对应0度
#define GIMBAL_MID_ANGLE 1500  // 中间占空比，对应180度
#define GIMBAL_MAX_ANGLE 2500  // 最大占空比，对应360度
#define GIMBAL_INIT_ANGLE 1000 // 初始位置占空比(未定义,随便填一个数字)
#define GIMBAL_DECK_GREEN_ANGLE 1001 // 云台绿色物料位置(未定义,随便填一个数字)
#define GIMBAL_DECK_RED_ANGLE 1002   // 云台红色物料位置(未定义,随便填一个数字)
#define GIMBAL_DECK_BLUE_ANGLE 1003  // 云台蓝色物料位置(未定义,随便填一个数字)
#define GIMBAL_WORK_GREEN_ANGLE 1004 // 云台绿色物料工作位置(未定义,随便填一个数字)
#define GIMBAL_WORK_RED_ANGLE 1005   // 云台红色物料工作位置(未定义,随便填一个数字)
#define GIMBAL_WORK_BLUE_ANGLE 1006  // 云台蓝色物料工作位置(未定义,随便填一个数字)


/*夹爪指定状态宏定义*/
#define GRIPPER_OPEN_ANGLE 500   // 夹爪张开占空比(未定义,随便填一个数字)
#define GRIPPER_CLOSE_ANGLE 2500 // 夹爪闭合占空比(未定义,随便填一个数字)

extern void bsp_servo_pwm_init(void);
extern void bsp_gimbal_angle_set(uint16_t duty);
extern void bsp_gripper_state_set(uint16_t duty);


#endif /* __BSP_SERVO_PWM_H */
