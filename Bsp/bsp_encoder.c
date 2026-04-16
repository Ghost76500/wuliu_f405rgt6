#include "bsp_encoder.h"
#include "tim.h"
#include <stdint.h>


encoder_motor_t encoder_motor[4];



/*-----------------------------------函数实现-----------------------------------*/

/**
 * @brief  初始化编码器
 * @param  none
 * @retval none
 */
void bsp_encoder_init(void)
{
	//HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
	//HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
	//HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
	//HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);

	// 上电后将编码器计数清零，避免首次读取到残留计数引起速度环输出抖动
	//__HAL_TIM_SET_COUNTER(&htim2, 0);
	//__HAL_TIM_SET_COUNTER(&htim3, 0);
	//__HAL_TIM_SET_COUNTER(&htim4, 0);
	//__HAL_TIM_SET_COUNTER(&htim5, 0);
}

/**
 * @brief  读取编码器脉冲计数
 * @param  htim 定时器句柄
 * @retval none
 */
static int16_t Read_Pulse_Count(TIM_HandleTypeDef *htim)
{
	int16_t temp;
	temp=(int16_t)__HAL_TIM_GetCounter(htim);
	__HAL_TIM_SetCounter(htim,0);
  
	return temp;
}

/**
 * @brief  增量式编码器速度读取
 * @param  motor_num 编码器编号
 * @retval none
 */
static int16_t read_speed_motor(uint8_t motor_num)
{
	int16_t encoder_delta;;
	switch(motor_num)
	{
		//case 1: encoder_delta = Read_Pulse_Count(&htim3);break; // 1
		//case 2: encoder_delta = -Read_Pulse_Count(&htim4);break; // 2
		//case 3: encoder_delta = Read_Pulse_Count(&htim2);break; // 3
		//case 4: encoder_delta = -Read_Pulse_Count(&htim5);break; // 4
		//default: encoder_delta = 0;break;
	}
	return encoder_delta;
}

/*
 * @brief  更新指定编码器速度
 * @param  motor_id 编码器编号
 * @retval none
 */
static void encoder_update_speed(uint8_t motor_id)
{
	if (motor_id < 1 || motor_id > 4)
	{
		return;
	}
	encoder_motor[motor_id - 1].speed = read_speed_motor(motor_id);
}

/*
 * @brief 
 * @param  none
 * @retval none
 */
void encoder_update_all(void)
{
	for (uint8_t i = 1; i <= 4; i++)
	{
		encoder_update_speed(i);
	}
}

int16_t encoder_get_speed(uint8_t motor_id)
{
	if (motor_id < 1 || motor_id > 4)
	{
		return 0;
	}
	return encoder_motor[motor_id - 1].speed;
}

