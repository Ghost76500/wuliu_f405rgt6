#ifndef BSP_USART_H
#define BSP_USART_H

#include "stdint.h"
#include "usart.h"

extern void usart_SendCmd(uint8_t *cmd, uint8_t length);
extern void uart_init_dma(UART_HandleTypeDef *huart);

#endif