#ifndef GM65_H
#define GM65_H

#include "stm32f4xx_hal.h"

extern void data_receive(UART_HandleTypeDef *huart);
extern void process_data(UART_HandleTypeDef *huart);
extern void uart_init_it(UART_HandleTypeDef *huart);
extern void uart_send(UART_HandleTypeDef *huart);

#endif // GM65_H