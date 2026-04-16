#include "bsp_led.h"
#include "stm32f4xx_hal.h"
#include "gpio.h"

/* 初始化 LED 引脚 */
void LED_Init(void)
{
    // 初始化 LED 引脚为低电平
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
}

/* 打开指定的 LED */
void bsp_led_on(uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_Pin, GPIO_PIN_SET);
}

void bsp_led_off(uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_Pin, GPIO_PIN_RESET);
}

void bsp_led_toggle(uint16_t GPIO_Pin)
{
    HAL_GPIO_TogglePin(GPIOC, GPIO_Pin);
}