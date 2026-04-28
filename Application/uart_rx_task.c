#include "uart_rx_task.h"
#include "cmsis_os2.h"
#include "command.h"
#include "gm65.h"
#include "bsp_usart.h"
#include "visual_identity.h"

// 声明 CubeMX 里创建的各个独立队列句柄
extern osMessageQueueId_t maxicamQueueHandle;
// extern osMessageQueueId_t opsQueueHandle;
extern uint8_t maxicam_rx_data[];

uint8_t visual_data_ready = 0; // 视觉数据就绪标志，供其他任务查询

void uart_rx_task(void *argument)
{
    osDelay(100); // 任务启动延时，等待系统稳定
    
    Uart_Msg_t rx_msg;            // 用于接收队列中碎片的变量
    uint8_t maxicam_frame[MAXICAM_FRAME_LENGTH];

    // 启动 Maxicam 的 DMA 接收监听！
    uart_init_dma(&huart2, maxicam_rx_data, MAX_UART_FRAG_LEN);

    for (;;)
    {
        if (osMessageQueueGet(maxicamQueueHandle, &rx_msg, NULL, 10) == osOK)
        {
            // 只要代码能走到这里，说明一定是被数据唤醒了
            // 下面正常进行环形缓冲区解析等操作...
            Command_Write(rx_msg.data, rx_msg.length);
            // ...
            while (Command_GetCommand(maxicam_frame) == 10)
            {
                // 在这里处理解析出的 Maxicam 数据
                // uint8_t info_type = maxicam_frame[1];
                // ...
                maixcam_data_unpack(maxicam_frame);
                calculate_error(); // 计算误差值，更新全局变量
                visual_data_ready = 1;
            }
        }
        else
        {
            // process_data(&huart5); // 处理gm65数据
        }
    }
}