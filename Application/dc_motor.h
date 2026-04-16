#ifndef DC_MOTOR_H
#define DC_MOTOR_H

#include <stdint.h>

/**
 * @brief 初始化电机PWM
 * @param 
 */
extern void dc_motor_pwm_init(void);

/**
 * @brief 单路电机PWM输出（默认实现使用 TIM3 CH1 + PB1 方向）
 * @param pwm PWM占空比（正反转由符号决定）
 */
extern void Motor_SetPWM(int16_t pwm);

/**
 * @brief 四轮底盘PWM接口，供 chassis_task 调用
 * @param pwm1 电机1 PWM
 * @param pwm2 电机2 PWM
 * @param pwm3 电机3 PWM
 * @param pwm4 电机4 PWM
 */
extern void pwm_cmd_chassis(int16_t pwm1, int16_t pwm2, int16_t pwm3, int16_t pwm4);

/**
 * @brief 各路电机PWM输出接口（可在其他文件重载以适配实际硬件）
 */
extern void Motor_SetPWM_M1(int16_t pwm1);
extern void Motor_SetPWM_M2(int16_t pwm2);
extern void Motor_SetPWM_M3(int16_t pwm3);
extern void Motor_SetPWM_M4(int16_t pwm4);

#endif // DC_MOTOR_H
