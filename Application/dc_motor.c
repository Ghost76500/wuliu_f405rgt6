#include "dc_motor.h"
#include "tim.h"

void dc_motor_pwm_init(void)
{
    //HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1); // 启动电机 PWM
    //HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2); // 启动电机 PWM
    //HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1); // 启动电机 PWM
    //HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2); // 启动电机 PWM
}


void Motor_SetPWM(int16_t pwm)
{
    
    if (pwm >= 0)
    { // 正转
        //__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (uint16_t)pwm);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET); // PB1低电平
    }
    else
    { // 反转
        //__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (uint16_t)(-pwm));
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);   // PB1高电平
    }
}


void pwm_cmd_chassis(int16_t pwm1, int16_t pwm2, int16_t pwm3, int16_t pwm4)
{
    Motor_SetPWM_M1(pwm1);
    Motor_SetPWM_M2(pwm2);
    Motor_SetPWM_M3(pwm3);
    Motor_SetPWM_M4(pwm4);
}

void Motor_SetPWM_M1(int16_t pwm1)
{
    if (pwm1 >= 0)
    { // 正转
        //__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, (uint16_t)pwm1);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET); // PE1低电平
    }
    else
    { // 反转
        //__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_2, (uint16_t)(-pwm1));
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);   // PE1高电平
    }
}

void Motor_SetPWM_M2(int16_t pwm2)
{
    if (pwm2 >= 0)
    { // 正转
        //__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, (uint16_t)pwm2);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET); // PE8低电平
    }
    else
    { // 反转
        //__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, (uint16_t)(-pwm2));
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET);   // PE8低电平
    }
}

void Motor_SetPWM_M3(int16_t pwm3)
{
    if (pwm3 >= 0)
    { // 正转
        //__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (uint16_t)pwm3);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET); // PA8低电平
    }
    else
    { // 反转
        //__HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, (uint16_t)(-pwm3));
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);   // PA8高电平
    }
}

void Motor_SetPWM_M4(int16_t pwm4)
{
    if (pwm4 >= 0)
    { // 正转
        //__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_1, (uint16_t)pwm4);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); // PC3低电平
    }
    else
    { // 反转
        //__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_1, (uint16_t)(-pwm4));
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);   // PC3高电平
    }
}