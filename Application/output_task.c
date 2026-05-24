#include "cmsis_os2.h"
#include "usart.h"
#include "chassis_task.h"
#include "position_task.h"
#include <stdio.h>


static char msg[64];
static void chassis_data_send(UART_HandleTypeDef *huart); // 发送底盘数据到上位机的函数声明，传入UART句柄

void output_task(void *argument)
{
    for (;;)
    {
        chassis_data_send(&huart2); // 传入UART2句柄，发送底盘数据到上位机
        osDelay(40);
    }
}

static void chassis_data_send(UART_HandleTypeDef *huart)
{
    if (huart == NULL)
    {
        return;
    }

    const chassis_odometry_t *odom = get_position_data();
    const chassis_move_t *chassis = get_chassis_move_data();

    if (odom == NULL || chassis == NULL)
    {
        return;
    }

    float pos_x = (float)odom->position_x;
    float pos_y = (float)odom->position_y;
    float yaw = (float)odom->yaw;
    float vx = (float)chassis->vx;
    float vy = (float)chassis->vy;
    float wz = (float)chassis->wz;

    int len = snprintf(msg, sizeof(msg),
                       "Data:%.3f,%.3f,%.3f\r\n",
                       pos_x, pos_y, yaw);
                       
    if (len > 0)
    {
        uint16_t send_len = (len < (int)sizeof(msg)) ? (uint16_t)len : (uint16_t)(sizeof(msg) - 1U);
        HAL_UART_Transmit_DMA(huart, (uint8_t *)msg, send_len);
    }
}