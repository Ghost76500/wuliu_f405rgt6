#include "bsp_servo_pwm.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "tower.h"

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

/**
 * @brief 占空比限幅函数
 * @param duty 占空比，范围500-2500，对应0-180度或0-360度
 */
uint16_t bsp_duty_limit(uint16_t duty)
{
    if (duty < 500) return 500;
    if (duty > 2500) return 2500;
    return duty;
}

/**
 * @brief 云台转动函数
 * @param num 角度编号
 * @param step_delay_ms 停顿时间
 */
void gimbal_turn(uint16_t num, uint16_t step_delay_ms)
{
    uint16_t angle = num;
    switch (angle)
    {
        case 1:
            angle = GIMBAL_ONE_ANGLE;
            break;
        case 2:
            angle = GIMBAL_TWO_ANGLE;
            break;
        case 3:
            angle = GIMBAL_THREE_ANGLE;
            break;
        case 4:
            angle = GIMBAL_BLUE_ANGLE; // 默认复位位置
            break;
        case 5:
            angle = GIMBAL_GREEN_ANGLE; // 默认复位位置
            break;
        case 6:
            angle = GIMBAL_RED_ANGLE; // 默认复位位置
            break;
        case 7:
            angle = GIMBAL_NAWULIAO_ANGLE; // 默认复位位置
            break;
        default:
            angle = GIMBAL_INIT_ANGLE; // 默认复位位置
    }
    bsp_gimbal_angle_set(angle);
    osDelay(step_delay_ms);
}
