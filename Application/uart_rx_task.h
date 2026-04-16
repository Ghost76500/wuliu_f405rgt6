#ifndef UART_RX_TASK_H
#define UART_RX_TASK_H

#include "stdint.h"
#include "struct_typedef.h"

// 定义一个统一的串口消息包裹
typedef struct {
    uint8_t uart_source; // 标记是谁发来的数据，比如 1代表UART1, 2代表UART2
    void *pData;         // 指向具体数据的指针
} Global_UART_Msg_t;

#endif // UART_RX_TASK_H