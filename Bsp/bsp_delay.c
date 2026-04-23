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


void DWT_Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void delay_us(uint32_t us)
{
    uint32_t delay_ticks = us * (SystemCoreClock / 1000000U);
    uint32_t start_tick = DWT->CYCCNT;
    while ((DWT->CYCCNT - start_tick) < delay_ticks)
    {
        __NOP(); 
    }
}