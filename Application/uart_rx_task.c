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
volatile float maxicam_pkt_hz = 0.0f; // Maxicam 数据包频率（Hz）

static void maxicam_freq_detect(void);

void uart_rx_task(void *argument)
{
    osDelay(100); // 任务启动延时，等待系统稳定
    
    Uart_Msg_t rx_msg;            // 用于接收队列中碎片的变量
    uint8_t maxicam_frame[MAXICAM_FRAME_LENGTH];

    // 启动 Maxicam 的 DMA 接收监听！
    uart_init_dma(&huart1, maxicam_rx_data, MAX_UART_FRAG_LEN);

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
                maxicam_freq_detect();
            }
        }
        else
        {
            // process_data(&huart5); // 处理gm65数据
        }
    }
}

static void maxicam_freq_detect(void)
{
    static uint32_t last_tick = 0U;
    static uint32_t frame_count = 0U;
    uint32_t now = osKernelGetTickCount();
    uint32_t tick_freq = osKernelGetTickFreq();

    frame_count++;

    if (tick_freq == 0U)
    {
        return;
    }

    if (last_tick == 0U)
    {
        last_tick = now;
        return;
    }

    if ((now - last_tick) >= tick_freq)
    {
        maxicam_pkt_hz = (float)frame_count * ((float)tick_freq / (float)(now - last_tick));
        frame_count = 0U;
        last_tick = now;
    }
}

/**
 * @brief  HAL库串口错误回调函数
 * @note   当串口发生错误（最常见的就是溢出错误 ORE）时，HAL库会自动调用这个函数。
 * 我们在这里进行“抢救”，把被锁死的串口重新救活。
 * @param  huart: 发生错误的串口句柄指针
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    // 判断是不是我们用于接收指令的串口2
    if (huart == &huart1)
    {
        // 1. 清除溢出错误标志位 (ORE)
        // 底层逻辑：如果不清除这个标志位，硬件会一直报警，再也无法触发正常的接收中断
        __HAL_UART_CLEAR_OREFLAG(huart);

        // 2. 强行解除 HAL 库的状态锁
        // 底层逻辑：HAL库内部在报错后，会把 RxState 设为错误状态，必须手动改回 READY
        huart->RxState = HAL_UART_STATE_READY;

        // 3. 重新开启串口空闲中断接收
        // 底层逻辑：让串口重新回到工作岗位，继续监听下一波数据
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, maxicam_rx_data, MAX_UART_FRAG_LEN);
        
    }
}

