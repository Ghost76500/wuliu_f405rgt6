#include "uart_rx_task.h"
#include "cmsis_os2.h"
#include "command.h"

// 声明 CubeMX 里创建的各个独立队列句柄
extern osMessageQueueId_t maxicamQueueHandle;
// extern osMessageQueueId_t opsQueueHandle;

void uart_rx_task(void *argument)
{
    osDelay(100); // 任务启动延时，等待系统稳定
    
    Uart_Msg_t rx_msg;            // 用于接收队列中碎片的变量

    for (;;)
    {
        // 第 4 个参数填 osWaitForever
        // 效果：如果队列空了，当前任务立刻交出 CPU 并在后台“沉睡”。
        // 直到串口中断（ISR）往队列里塞了数据，RTOS 会瞬间把这个任务“踹醒”并继续往下执行。
        if (osMessageQueueGet(maxicamQueueHandle, &rx_msg, NULL, osWaitForever) == osOK)
        {
            // 只要代码能走到这里，说明一定是被数据唤醒了
            // 下面正常进行环形缓冲区解析等操作...
            Command_Write(rx_msg.data, rx_msg.length);
            // ...
        }
    }
}