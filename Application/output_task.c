#include "cmsis_os2.h"
#include "main.h"
#include "chassis_task.h"
#include "position_task.h"
#include "stm32f4xx_hal_uart.h"

static char msg[128];
static void chassis_data_send(UART_HandleTypeDef *huart); // 发送底盘数据到上位机的函数声明，传入UART句柄

void output_task(void *argument)
{
    //chassis_data_send(&huart2); // 传入UART2句柄，发送底盘数据到上位机
    osDelay(40); // 任务启动延时，等待其他任务完成初始化
}

static void chassis_data_send(UART_HandleTypeDef *huart)
{
    
}