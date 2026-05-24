#include "bsp_usart.h"
#include "bsp_led.h"
#include <stdint.h>
#include "gm65.h"
#include <string.h>
#include "cmsis_os2.h"
#include "uart_rx_task.h"

extern osMessageQueueId_t maxicamQueueHandle;
extern uint8_t dma_rx_buffer[]; // 引用底层数组

uint8_t zdt_rx_data[32] = {0}; // DMA接收缓冲区
uint8_t blue_rx_data[64] = {0}; // 蓝牙DMA接收缓冲区

uint8_t dma_rx_buffer[MAX_UART_FRAG_LEN]; // // 定义给 DMA 硬件无脑搬运使用的底层数组，maixcam用

static uint8_t uart3_tx_buf[32]; // 定义一个底层发送缓冲区，给 usart_SendCmd 函数使用

// 【新增】：为 Maxicam 准备专属的底层缓冲区
uint8_t maxicam_rx_data[MAX_UART_FRAG_LEN] = {0};

void usart_SendCmd(uint8_t *cmd, uint8_t length)
{
    for (int i = 0; i < length; i++) {
        uart3_tx_buf[i] = cmd[i];
    }
    HAL_UART_Transmit_DMA(&huart3, uart3_tx_buf, length);
}

void uart_init_dma(UART_HandleTypeDef *huart, uint8_t *buffer, uint16_t size)
{
    HAL_UARTEx_ReceiveToIdle_DMA(huart, buffer, size);
    __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT); // 关掉半满中断，防止干扰
}

// huart4 DMA接收空闲中断
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == UART4)
    {
        // 重新启动UART4 DMA接收 关闭
        HAL_UARTEx_ReceiveToIdle_DMA(&huart4, zdt_rx_data, sizeof(zdt_rx_data));
        __HAL_DMA_DISABLE_IT(huart4.hdmarx, DMA_IT_HT);
    }

    if (huart->Instance == USART1)
    {
        // 1. 拿出一个空的快递纸箱 (在栈上分配，退出函数自动销毁)
        Uart_Msg_t tx_msg; 
        
        // 2. 填写快递单（记录本次收到了几个字节）
        tx_msg.length = Size;
        
        // 3. 把 DMA 刚收到的数据装进纸箱 (深拷贝)
        memcpy(tx_msg.data, dma_rx_buffer, Size);
        
        // 4. 将纸箱投递进 FreeRTOS 队列中
        // 参数3：优先级，填 0 即可
        // 参数4：超时时间，【注意】在中断 ISR 里面调用此函数，超时时间必须强制填 0！
        osMessageQueuePut(maxicamQueueHandle, &tx_msg, 0, 0);
        
        // 5. 快递发走后，立刻开启下一次 DMA 空闲中断接收，准备接下一单！
        HAL_UARTEx_ReceiveToIdle_DMA(huart, dma_rx_buffer, MAX_UART_FRAG_LEN);
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