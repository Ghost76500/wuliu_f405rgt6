#include "bsp_servo_pwm.h"
#include "tim.h"
#include "cmsis_os2.h"

/**
 * @brief 初始化云台和夹爪舵机 PWM
 */
void bsp_servo_pwm_init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
}

void bsp_test_angle_set(uint16_t duty)
{
    if (duty < 500) duty = 500;
    if (duty > 2500) duty = 2500;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, duty);
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

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, duty);
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

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
}

void bsp_gimbal_turn(uint16_t target_duty, uint16_t step_delay_ms)
{
    bsp_gimbal_angle_set(target_duty);
    osDelay(step_delay_ms);
}
