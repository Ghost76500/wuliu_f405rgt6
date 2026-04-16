#include "bsp_servo_pwm.h"
#include "tim.h"

/**
 * @brief 初始化云台和夹爪舵机 PWM
 */
void bsp_servo_pwm_init(void)
{
    // 启动云台舵机 PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    // 启动夹爪舵机 PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
}


/**
 * @brief 设置云台舵机角度
 * @param duty 占空比，范围500-2500，对应0-360度
 */
void bsp_gimbal_angle_set(uint16_t duty)
{
    // 范围是500到2500，对应0到360度
    if (duty < 500) duty = 500;
    if (duty > 2500) duty = 2500;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
}

/**
 * @brief 设置夹爪舵机状态
 * @param duty 占空比，范围500-2500，对应0-180度
 */
void bsp_gripper_state_set(uint16_t duty)
{
    // 范围是500到2500，对应0到180度
    if (duty < 500) duty = 500;
    if (duty > 2500) duty = 2500;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, duty);
}

