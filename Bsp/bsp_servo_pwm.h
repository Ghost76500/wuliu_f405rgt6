#ifndef __BSP_SERVO_PWM_H
#define __BSP_SERVO_PWM_H

#include "stdint.h"

/*云台指定角度宏定义*/
#define GIMBAL_MIN_ANGLE 500   // 最小占空比，对应0度
#define GIMBAL_MID_ANGLE 1500  // 中间占空比，对应180度
#define GIMBAL_MAX_ANGLE 2500  // 最大占空比，对应360度

#define GIMBAL_INIT_ANGLE 1960 // 初始位置占空比(未定义,随便填一个数字)

#define GIMBAL_ONE_ANGLE 2320 // 云台转动一个角度对应的占空比增量 中间差170
#define GIMBAL_TWO_ANGLE 2150 // 测试用占空比
#define GIMBAL_THREE_ANGLE 1980 // 测试用占空比

#define GIMBAL_BLUE_ANGLE 1265
#define GIMBAL_GREEN_ANGLE 960
#define GIMBAL_RED_ANGLE 665

#define GIMBAL_NAWULIAO_ANGLE 960


/*夹爪指定状态宏定义*/
#define JIAZHUA_INIT 500   // 夹爪初始状态占空比(未定义,随便填一个数字)
#define JIAZHUA_CLOSE 520   // 夹爪闭合占空比(未定义,随便填一个数字)
#define JIAZHUA_XIAO_OPEN 650   // 夹爪张开一点点占空比(未定义,随便填一个数字)
#define JIAZHUA_DA_OPEN 850   // 夹爪完全张开占空比

extern void bsp_servo_pwm_init(void);
extern void bsp_gimbal_angle_set(uint16_t duty);
extern void bsp_gripper_state_set(uint16_t duty);
extern void bsp_test_angle_set(uint16_t duty);
extern void gimbal_turn(uint16_t num, uint16_t step_delay_ms);

#endif /* __BSP_SERVO_PWM_H */
