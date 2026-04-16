/**
 * @file bsp_delay.c
 * @brief 
 * ----------------------------------------------------------------------------
 * @version 1.0.0.0
 * @author RM
 * @date 2018-12-26
 * @remark 官步初始代码
 * ----------------------------------------------------------------------------
 * @version 1.0.0.1
 * @author 周明杨
 * @date 2024-12-30
 * @remark 优化整体架构
 */

#include "bsp_delay.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "cmsis_os2.h"

void delay_ms(uint16_t ms)
{
    HAL_Delay(ms);
}

void delay_us(uint16_t us)
{
    // 1. 设置计数器为0 
    //__HAL_TIM_SET_COUNTER(&htim13, 0);

    // 2. 开启定时器
    //HAL_TIM_Base_Start(&htim13);

    // 3. 等待计数到达指定值
    // "傻等"模式：只要计数器的值小于设定的 us，就一直在这里空转
    // 这里的计数器每 1us 会自动加 1
    //while (__HAL_TIM_GET_COUNTER(&htim13) < us);

    // 4. 关闭定时器
    //HAL_TIM_Base_Stop(&htim13);
}