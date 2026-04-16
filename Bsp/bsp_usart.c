#include "bsp_usart.h"
#include "bsp_led.h"
#include <stdint.h>
#include "gm65.h"
#include <string.h>


uint8_t zdt_rx_data[32] = {0}; // DMA接收缓冲区
uint8_t blue_rx_data[64] = {0}; // 蓝牙DMA接收缓冲区

static uint8_t uart3_tx_buf[32];
static uint8_t uart4_tx_buf[32];

void usart_SendCmd(uint8_t *cmd, uint8_t length)
{
    for (int i = 0; i < length; i++) {
        uart4_tx_buf[i] = cmd[i];
    }
    HAL_UART_Transmit_DMA(&huart4, uart4_tx_buf, length);
}

void uart_init_dma(UART_HandleTypeDef *huart)
{
    HAL_UARTEx_ReceiveToIdle_DMA(huart, zdt_rx_data, sizeof(zdt_rx_data));
    __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);
}

// huart4 DMA接收空闲中断
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == UART4)
    {
        // 处理接收完成的逻辑
        // uint8_t rx_data[256] = {0};

        // 复制到处理缓冲区
        for (uint16_t i = 0; i < Size; i++)
        {
            uart3_tx_buf[i] = zdt_rx_data[i];
        }
        
        // 清空接收缓冲区
        //for (uint16_t i = 0; i < sizeof(zdt_rx_data); i++)
        //{
        //    zdt_rx_data[i] = 0;
        //}

        bsp_led_toggle(CORE_THREE);
        // HAL_UART_Transmit_DMA(&huart3, uart3_tx_buf, Size); // 发送接收到的数据

        // 重新启动UART4 DMA接收 关闭
        HAL_UARTEx_ReceiveToIdle_DMA(&huart4, zdt_rx_data, sizeof(zdt_rx_data));
        __HAL_DMA_DISABLE_IT(huart4.hdmarx, DMA_IT_HT);
    }

    if (huart->Instance == USART1)
    {
        
    }

    if (huart->Instance == USART3)
    {

    }
}

// 串口接收中断
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart5) // 检查是否为 USART5
    {
        data_receive(&huart5); // 调用数据接收处理函数
    }
}