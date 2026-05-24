#include "gm65.h"
#include "bsp_led.h"
#include "stm32f4xx_hal_uart.h"
#include "usart.h"


uint8_t rx_data[256] = {0}; // 接收缓冲区
uint8_t tx_data[] = {0x7E,0x00,0x07,0x01,0x00,0x0A,0x01,0xEE,0x8A};
// uint8_t tx_test[] = "Hello GM65!";

static uint8_t rx_byte = 0;          // 每次中断接收1字节
static volatile uint8_t rx_index = 0;         // 记录已接收字节数
static volatile uint8_t data_ready = 0;       // 数据包完成标志

uint8_t color_task[7]={0};

void uart_init_it(UART_HandleTypeDef *huart)
{
    // 启动第一次接收中断
    HAL_UART_Receive_IT(huart, &rx_byte, 1);
}

void data_receive(UART_HandleTypeDef *huart) // 传入uart句柄
{
    // 每收到1字节，存入缓冲区

    rx_data[rx_index] = rx_byte;
    rx_index++;
    
    // 检测 \r\n（数据包结束）
    if (rx_index >= 2 && rx_data[rx_index-2] == '\r' && rx_data[rx_index-1] == '\n')
    {
      data_ready = 1;  // 标记数据完整，由主循环处理
      // 拷贝数据到color_task数组
        for (int i = 0; i < 7; i++)
        {
            color_task[i] = rx_data[i];
        }
      return;
    }
    
    // 继续接收下一个字节
    HAL_UART_Receive_IT(huart, &rx_byte, 1);
}

// 清空数组并重置状态，准备接收下一条数据，不需要用到
void process_data(UART_HandleTypeDef *huart)
{
    if (data_ready)
    {
        data_ready = 0;
        // 重置缓冲区，等待下一条数据
        rx_index = 0;

        for (int i = 0; i < sizeof(rx_data); i++)
        {
            rx_data[i] = 0;
        }
        HAL_UART_Receive_IT(huart, &rx_byte, 1);
    }
}



void uart_send(UART_HandleTypeDef *huart)
{
    HAL_UART_Transmit_IT(huart, tx_data, sizeof(tx_data));
}