#ifndef UART_RX_TASK_H
#define UART_RX_TASK_H

#include "stdint.h"

#define MAX_UART_FRAG_LEN 32

extern uint8_t visual_data_ready; // 视觉数据就绪标志，供其他任务查询

// 基础的数据包结构体 (取消了 uart_source，因为每个外设有专属队列)
typedef struct {
    uint16_t length;                  // 数据长度
    uint8_t  data[MAX_UART_FRAG_LEN]; // 数据本体，深拷贝最安全
} Uart_Msg_t;

#endif // UART_RX_TASK_H