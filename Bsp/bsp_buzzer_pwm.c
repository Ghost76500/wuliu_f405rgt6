#include "bsp_buzzer_pwm.h"
#include "tim.h"
#include "cmsis_os2.h"

void buzzer_init(void)
{
    // 使用 TIM14 的通道 1 作为蜂鸣器 PWM 输出
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    
    // 初始设置为静音
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0);
}

/* * 该函数用于设置蜂鸣器的频率和音量
 * * 参数:
 * frequency: 声音频率 (Hz)，比如 2000 代表 2kHz
 * volume:    音量 (0 - 100)，0为静音，100为最大音量
 */
void buzzer_control(uint16_t frequency, uint8_t volume)
{
    if (frequency == 0 || volume == 0)
    {
        // 静音处理
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0); 
        return;
    }

    // 计算 ARR (自动重装载值)
    // PSC 是 71，时钟源 72MHz，计数频率是 1MHz
    uint32_t arr_value = (1000000 / frequency) - 1;

    // 修改自动重装载寄存器的值
    __HAL_TIM_SET_AUTORELOAD(&htim4, arr_value);

    // 计算 CCR (占空比)
    float duty_ratio = (float)volume / 200.0f; 
    uint32_t ccr_value = (uint32_t)((arr_value + 1) * duty_ratio);

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, ccr_value);
}

void buzzer_rings(uint16_t frequency, uint8_t volume, uint16_t ring_time_ms)
{
    buzzer_control(frequency, volume);
    osDelay(ring_time_ms);
    buzzer_control(0, 0); // 静音
}