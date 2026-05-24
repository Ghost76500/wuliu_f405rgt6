#ifndef __BSP_SERVO_PWM_H
#define __BSP_SERVO_PWM_H

#include "stdint.h"


extern void bsp_servo_pwm_init(void);
extern void bsp_gimbal_angle_set(uint16_t duty);
extern void bsp_gripper_state_set(uint16_t duty);
extern void bsp_test_angle_set(uint16_t duty);
extern void gimbal_turn(uint16_t num, uint16_t step_delay_ms);

#endif /* __BSP_SERVO_PWM_H */
