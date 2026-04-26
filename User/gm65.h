#ifndef GM65_H
#define GM65_H

#include "stm32f4xx_hal.h"

extern uint8_t color_task[6]; // 用于存储接收到的数据，供其他任务使用

extern void data_receive(UART_HandleTypeDef *huart);
extern void process_data(UART_HandleTypeDef *huart);
extern void uart_init_it(UART_HandleTypeDef *huart);
extern void uart_send(UART_HandleTypeDef *huart);

#endif // GM65_H